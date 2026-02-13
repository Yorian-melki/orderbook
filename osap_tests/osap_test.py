"""
OSAP Portfolio Returns Test - Le Raccourci

Tests 5 key signals using OSAP decile returns:
- BM (Book-to-Market)
- Mom12m (12-month Momentum)
- GP (Gross Profitability)
- Beta
- Accruals

Steps:
1. Download OSAP decile returns (equal-weight)
2. Download Ken French FF5+UMD monthly
3. Run alpha regressions vs FF5+UMD
4. Build composite (top decile, value+momentum+quality)
5. Backtest 2000-2024 with 20bps transaction costs
6. Rank signals by alpha, t-stat, post-pub survival
"""

import openassetpricing as oap
import pandas as pd
import numpy as np
from datetime import datetime
import pandas_datareader as pdr
import warnings
warnings.filterwarnings('ignore')

print("=" * 80)
print("OSAP PORTFOLIO RETURNS TEST - LE RACCOURCI")
print("=" * 80)

# Configuration
SIGNALS = ['BM', 'Mom12m', 'GP', 'Beta', 'Accruals']
START_DATE = '2000-01-01'
END_DATE = '2024-12-31'
TRANSACTION_COST_BPS = 20

print(f"\nSignals: {', '.join(SIGNALS)}")
print(f"Period: {START_DATE} to {END_DATE}")
print(f"Transaction costs: {TRANSACTION_COST_BPS} bps")

# Step 1: Download OSAP decile returns (equal-weight)
print("\n" + "=" * 80)
print("STEP 1: Downloading OSAP Decile Returns (Equal-Weight)")
print("=" * 80)

client = oap.OpenAP()
print("\nDownloading all decile portfolios from OSAP...")
df_all = client.dl_port('deciles_ew', 'pandas')
print(f"  ✓ Downloaded {len(df_all)} observations for {df_all['signalname'].nunique()} signals")

# Extract our target signals
decile_returns = {}
for signal in SIGNALS:
    print(f"\nExtracting {signal}...")

    # Filter for this signal and decile 10 (top decile)
    signal_data = df_all[(df_all['signalname'] == signal) & (df_all['port'] == '10')].copy()

    if len(signal_data) > 0:
        # Convert date to datetime and normalize to month start for merging with FF factors
        signal_data['date'] = pd.to_datetime(signal_data['date'])
        # Convert to first of month (FF factors use month start)
        signal_data['date'] = signal_data['date'].dt.to_period('M').dt.to_timestamp()

        # Select relevant columns and rename
        signal_df = signal_data[['date', 'ret']].copy()
        signal_df.columns = ['date', 'dec_10']

        # Convert returns from % to decimal
        signal_df['dec_10'] = signal_df['dec_10'] / 100

        print(f"  ✓ {signal}: {len(signal_df)} observations")
        decile_returns[signal] = signal_df
    else:
        print(f"  ✗ {signal}: No data found")

print(f"\nSuccessfully extracted {len(decile_returns)}/{len(SIGNALS)} signals")

# Step 2: Download Ken French FF5+UMD monthly
print("\n" + "=" * 80)
print("STEP 2: Downloading Ken French FF5+UMD Factors")
print("=" * 80)

print("\nDownloading Fama-French 5 factors...")
try:
    ff5 = pdr.get_data_famafrench('F-F_Research_Data_5_Factors_2x3', start='1900-01-01')[0]
    # The index is already in period format, convert to timestamp
    ff5.index = ff5.index.to_timestamp()
    ff5 = ff5.reset_index()
    ff5.columns = ['date', 'Mkt-RF', 'SMB', 'HML', 'RMW', 'CMA', 'RF']

    # Convert from % to decimal
    for col in ff5.columns:
        if col != 'date':
            ff5[col] = ff5[col] / 100

    print(f"  ✓ FF5: {len(ff5)} observations")
except Exception as e:
    print(f"  ✗ Error downloading FF5: {str(e)}")
    import traceback
    traceback.print_exc()
    ff5 = None

print("\nDownloading Momentum factor...")
try:
    mom = pdr.get_data_famafrench('F-F_Momentum_Factor', start='1900-01-01')[0]
    # The index is already in period format, convert to timestamp
    mom.index = mom.index.to_timestamp()
    mom = mom.reset_index()
    mom.columns = ['date', 'UMD']

    # Convert from % to decimal
    mom['UMD'] = mom['UMD'] / 100

    print(f"  ✓ UMD: {len(mom)} observations")
except Exception as e:
    print(f"  ✗ Error downloading UMD: {str(e)}")
    import traceback
    traceback.print_exc()
    mom = None

# Merge FF5 and UMD
if ff5 is not None and mom is not None:
    ff_factors = pd.merge(ff5, mom, on='date', how='inner')
    print(f"\n  ✓ FF5+UMD merged: {len(ff_factors)} observations")
    print(f"  Columns: {', '.join(ff_factors.columns.tolist())}")
else:
    ff_factors = None
    print(f"\n  ✗ Could not create FF5+UMD factors")

# Step 3: Run alpha regressions for each signal
print("\n" + "=" * 80)
print("STEP 3: Running Alpha Regressions vs FF5+UMD")
print("=" * 80)

results = []

if ff_factors is not None:
    for signal, df in decile_returns.items():
        print(f"\n{signal}:")

        try:
            # Merge with FF factors
            merged = pd.merge(df, ff_factors, on='date', how='inner')

            # Filter date range
            merged = merged[(merged['date'] >= START_DATE) & (merged['date'] <= END_DATE)]

            if len(merged) < 24:  # Need at least 2 years of data
                print(f"  ✗ Insufficient data: {len(merged)} months")
                continue

            # Calculate excess returns
            merged['excess_ret'] = merged['dec_10'] - merged['RF']

            # Run regression: excess_ret ~ Mkt-RF + SMB + HML + RMW + CMA + UMD
            from sklearn.linear_model import LinearRegression
            from scipy import stats

            # Prepare factors
            factor_cols = ['Mkt-RF', 'SMB', 'HML', 'RMW', 'CMA', 'UMD']
            available_factors = [col for col in factor_cols if col in merged.columns]

            X = merged[available_factors].values
            y = merged['excess_ret'].values

            # Fit regression
            reg = LinearRegression()
            reg.fit(X, y)

            # Calculate alpha (annualized)
            alpha_monthly = reg.intercept_
            alpha_annual = alpha_monthly * 12 * 100  # Convert to annual %

            # Calculate residuals and standard errors
            y_pred = reg.predict(X)
            residuals = y - y_pred
            n = len(y)
            k = len(available_factors)

            # Standard error of alpha
            mse = np.sum(residuals**2) / (n - k - 1)
            X_with_const = np.column_stack([np.ones(n), X])
            se_alpha = np.sqrt(mse * np.linalg.inv(X_with_const.T @ X_with_const)[0, 0])

            # T-statistic (annualized)
            t_stat = alpha_monthly / se_alpha * np.sqrt(12)

            # Calculate Sharpe ratio
            mean_excess = merged['excess_ret'].mean()
            std_excess = merged['excess_ret'].std()
            sharpe_monthly = mean_excess / std_excess if std_excess > 0 else 0
            sharpe_annual = sharpe_monthly * np.sqrt(12)

            # Determine post-publication period (rough estimates)
            pub_dates = {
                'BM': '1992-01-01',
                'Mom12m': '1993-01-01',
                'GP': '2013-01-01',
                'Beta': '1972-01-01',
                'Accruals': '1996-01-01'
            }

            pub_date = pub_dates.get(signal, START_DATE)
            post_pub = merged[merged['date'] >= pub_date]

            if len(post_pub) >= 24:
                # Run post-pub regression
                X_post = post_pub[available_factors].values
                y_post = post_pub['excess_ret'].values

                reg_post = LinearRegression()
                reg_post.fit(X_post, y_post)

                alpha_post_monthly = reg_post.intercept_
                alpha_post_annual = alpha_post_monthly * 12 * 100

                # Calculate t-stat for post-pub
                y_pred_post = reg_post.predict(X_post)
                residuals_post = y_post - y_pred_post
                n_post = len(y_post)
                mse_post = np.sum(residuals_post**2) / (n_post - k - 1)
                X_post_const = np.column_stack([np.ones(n_post), X_post])
                se_alpha_post = np.sqrt(mse_post * np.linalg.inv(X_post_const.T @ X_post_const)[0, 0])
                t_stat_post = alpha_post_monthly / se_alpha_post * np.sqrt(12)

                survival_ratio = alpha_post_annual / alpha_annual if alpha_annual != 0 else 0
            else:
                alpha_post_annual = np.nan
                t_stat_post = np.nan
                survival_ratio = np.nan

            print(f"  Alpha (annual): {alpha_annual:.2f}% (t={t_stat:.2f})")
            print(f"  Sharpe ratio: {sharpe_annual:.2f}")
            print(f"  Post-pub alpha: {alpha_post_annual:.2f}% (t={t_stat_post:.2f})")
            print(f"  Survival ratio: {survival_ratio:.2%}")

            results.append({
                'Signal': signal,
                'Alpha (%)': alpha_annual,
                't-stat': t_stat,
                'Sharpe': sharpe_annual,
                'Post-pub Alpha (%)': alpha_post_annual,
                'Post-pub t-stat': t_stat_post,
                'Survival Ratio': survival_ratio,
                'N months': len(merged)
            })

        except Exception as e:
            print(f"  ✗ Error: {str(e)}")
            import traceback
            traceback.print_exc()

# Step 4 & 5: Build composite and backtest
print("\n" + "=" * 80)
print("STEP 4-5: Building Composite and Backtesting (2000-2024, 20bps costs)")
print("=" * 80)

try:
    # Select value, momentum, and quality signals
    value_signal = 'BM'
    momentum_signal = 'Mom12m'
    quality_signals = ['GP', 'Accruals']  # Use both GP and Accruals for quality

    # Get top decile returns for each
    composite_dfs = []

    for signal in [value_signal, momentum_signal] + quality_signals:
        if signal in decile_returns:
            df = decile_returns[signal].copy()
            df.columns = ['date', signal]  # Rename dec_10 to signal name
            composite_dfs.append(df)

    if len(composite_dfs) >= 3:
        # Merge all signals
        composite = composite_dfs[0]
        for df in composite_dfs[1:]:
            composite = pd.merge(composite, df, on='date', how='inner')

        # Equal-weight composite
        signal_cols = [col for col in composite.columns if col != 'date']
        composite['composite_ret'] = composite[signal_cols].mean(axis=1)

        # Filter date range
        composite = composite[(composite['date'] >= START_DATE) & (composite['date'] <= END_DATE)]

        # Apply transaction costs (monthly rebalance = 20bps each month)
        # Convert bps to decimal: 20bps = 0.20% = 0.0020
        composite['net_ret'] = composite['composite_ret'] - (TRANSACTION_COST_BPS / 10000.0)

        # Calculate cumulative returns
        composite['cum_gross'] = (1 + composite['composite_ret']).cumprod()
        composite['cum_net'] = (1 + composite['net_ret']).cumprod()

        # Performance metrics
        total_gross = (composite['cum_gross'].iloc[-1] - 1) * 100
        total_net = (composite['cum_net'].iloc[-1] - 1) * 100

        n_years = len(composite) / 12
        cagr_gross = ((1 + composite['composite_ret']).prod() ** (12 / len(composite)) - 1) * 100
        cagr_net = ((1 + composite['net_ret']).prod() ** (12 / len(composite)) - 1) * 100

        vol_annual = composite['composite_ret'].std() * np.sqrt(12) * 100
        sharpe_gross = (composite['composite_ret'].mean() / composite['composite_ret'].std()) * np.sqrt(12)
        sharpe_net = (composite['net_ret'].mean() / composite['net_ret'].std()) * np.sqrt(12)

        # Max drawdown
        cum_max = composite['cum_net'].expanding().max()
        drawdown = (composite['cum_net'] - cum_max) / cum_max
        max_dd = drawdown.min() * 100

        print(f"\nComposite: {value_signal} + {momentum_signal} + Quality({', '.join(quality_signals)})")
        print(f"Period: {composite['date'].min()} to {composite['date'].max()} ({len(composite)} months)")
        print(f"\nGROSS RETURNS:")
        print(f"  Total return: {total_gross:.2f}%")
        print(f"  CAGR: {cagr_gross:.2f}%")
        print(f"  Volatility: {vol_annual:.2f}%")
        print(f"  Sharpe ratio: {sharpe_gross:.2f}")
        print(f"\nNET RETURNS (after 20bps monthly costs):")
        print(f"  Total return: {total_net:.2f}%")
        print(f"  CAGR: {cagr_net:.2f}%")
        print(f"  Sharpe ratio: {sharpe_net:.2f}")
        print(f"  Max drawdown: {max_dd:.2f}%")

        # Add composite to results
        results.append({
            'Signal': 'COMPOSITE',
            'Alpha (%)': np.nan,
            't-stat': np.nan,
            'Sharpe': sharpe_net,
            'Post-pub Alpha (%)': np.nan,
            'Post-pub t-stat': np.nan,
            'Survival Ratio': np.nan,
            'N months': len(composite)
        })

except Exception as e:
    print(f"Error building composite: {str(e)}")
    import traceback
    traceback.print_exc()

# Step 6: Create ranked results table
print("\n" + "=" * 80)
print("STEP 6: RANKED RESULTS TABLE")
print("=" * 80)

if results:
    results_df = pd.DataFrame(results)

    # Sort by alpha (descending)
    results_df = results_df.sort_values('Alpha (%)', ascending=False)

    print("\n" + results_df.to_string(index=False))

    # Save results
    output_file = '/Users/yorian/osap_test_results.csv'
    results_df.to_csv(output_file, index=False)
    print(f"\n✓ Results saved to: {output_file}")
else:
    print("\nNo results to display")

print("\n" + "=" * 80)
print("TEST COMPLETE")
print("=" * 80)
