import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import struct
from scipy import stats

# Hardcoded filepath - edit this to point to your CSV file
FILEPATH = r"..\..\data\scope\X sweep 2.csv"


def plot_voltage_vs_time():
    """
    Plot voltage vs timestamp from a CSV file.
    The CSV file should have two columns: voltage and timestamp (no headers).
    """
    # Read the CSV file without headers
    df = pd.read_csv(FILEPATH, header=None, delimiter=';')
    
    # Create the plot using column indices (0 = first column, 1 = second column)
    plt.figure(figsize=(12, 6))
    plt.plot(df[0], df[1], linewidth=1)
    plt.xlabel('Timestamp')
    plt.ylabel('Voltage')
    plt.title('Voltage vs Timestamp')
    plt.grid(True, alpha=0.3)
    plt.tight_layout()
    plt.show()


def create_voltage_sweep(num_points, axis="x"):
    """
    Create a voltage sweep binfile with a given number of points and range.
    The binfile will be saved to the data/scope directory.
    """
    voltage_up_range = np.linspace(0, 65535, num_points//2).astype(np.uint16)
    voltage_down_range = np.linspace(65535, 0, num_points//2).astype(np.uint16)
    voltage_range = np.concatenate((voltage_up_range, voltage_down_range))

    if axis == "x":
        arr = np.column_stack((voltage_range, np.zeros(num_points, dtype=np.uint16), np.zeros(num_points, dtype=np.uint8)))
    elif axis == "y":
        arr = np.column_stack((np.zeros(num_points, dtype=np.uint16), voltage_range, np.zeros(num_points, dtype=np.uint8)))
    else:
        raise ValueError("Invalid axis")

    bin_file = open(f"../../data/scope/{axis}_voltage_sweep_{num_points}.bin", "wb")

    for i in range(num_points):
        bin_file.write(struct.pack(">HHB", arr[i][0], arr[i][1], arr[i][2]))
    bin_file.close()
    print(f"Voltage sweep saved to ../../data/scope/{axis}_voltage_sweep_{num_points}.bin")
    return arr

def analyze_and_fit_polynomials(time_threshold_lower, time_threshold_upper, dac_range=65536, plotting=False):
    """
    Analyze voltage data by:
    1. Filtering data where time_threshold_lower <= timestamp < time_threshold_upper
    2. Mapping voltage measurements to their corresponding DAC values
    3. Averaging voltage values for each DAC value
    4. Fitting 1st, 2nd, and 3rd order polynomials (DAC vs Voltage)
    5. Plotting and printing detailed results
    
    Parameters:
    -----------
    time_threshold_lower : float
        Lower time threshold for filtering data
    time_threshold_upper : float
        Upper time threshold for filtering data
    dac_range : int
        Number of DAC values in the sweep (e.g., 65536 for 16-bit DAC)
    """
    # Read the CSV file without headers
    df = pd.read_csv(FILEPATH, header=None, delimiter=';')
    
    # Rename columns for clarity
    df.columns = ['timestamp', 'voltage']
    
    # Filter data by time thresholds
    filtered_df = df[(df['timestamp'] >= time_threshold_lower) & 
                     (df['timestamp'] < time_threshold_upper)].copy()
    
    # print("="*80)
    # print("DAC-TO-VOLTAGE CALIBRATION ANALYSIS")
    # print("="*80)
    # print(f"\nOriginal dataset size: {len(df)} points")
    # print(f"Time window: [{time_threshold_lower:.6f}, {time_threshold_upper:.6f}]")
    # print(f"Filtered dataset size: {len(filtered_df)} points")
    # print(f"Percentage retained: {100 * len(filtered_df) / len(df):.2f}%")
    # print(f"\nTimestamp range: [{filtered_df['timestamp'].min():.6f}, {filtered_df['timestamp'].max():.6f}]")
    # print(f"Voltage range: [{filtered_df['voltage'].min():.6f}, {filtered_df['voltage'].max():.6f}]")
    # print(f"DAC range: [0, {dac_range-1}] ({dac_range} values)")
    
    # print(f"\n{'='*80}")
    # print(f"MAPPING DATA TO DAC VALUES")
    # print(f"{'='*80}")
    
    # Map each data point to its corresponding DAC value
    # Assuming DAC sweep goes linearly from 0 to dac_range-1 over the filtered time window
    dac_values = np.linspace(0, dac_range - 1, len(filtered_df))
    
    # Assign DAC values to the filtered dataframe
    filtered_df['dac_value'] = dac_values
    
    # Group by DAC value (rounded to integers) and average the voltages
    filtered_df['dac_int'] = np.round(dac_values).astype(int)
    
    # Calculate average voltage for each DAC value
    grouped = filtered_df.groupby('dac_int')['voltage'].agg(['mean', 'count', 'std']).reset_index()
    grouped.columns = ['dac', 'voltage_mean', 'count', 'voltage_std']
    
    # Create arrays with all DAC values (fill missing ones with interpolation)
    all_dac_values = np.arange(dac_range)
    all_voltages = np.zeros(dac_range)
    
    # Fill in measured values
    for _, row in grouped.iterrows():
        dac_idx = int(row['dac'])
        if 0 <= dac_idx < dac_range:
            all_voltages[dac_idx] = row['voltage_mean']
    
    # Interpolate missing DAC values
    measured_dacs = grouped['dac'].values
    measured_voltages = grouped['voltage_mean'].values
    missing_mask = np.ones(dac_range, dtype=bool)
    missing_mask[measured_dacs] = False
    
    if np.any(missing_mask):
        all_voltages[missing_mask] = np.interp(all_dac_values[missing_mask], 
                                                measured_dacs, 
                                                measured_voltages)
    
    # print(f"Total DAC values: {dac_range}")
    # print(f"DAC values with measurements: {len(grouped)}")
    # print(f"DAC values interpolated: {np.sum(missing_mask)}")
    # print(f"Average measurements per DAC: {grouped['count'].mean():.2f}")
    # print(f"\nVoltage statistics:")
    # print(f"  Mean: {all_voltages.mean():.6f}")
    # print(f"  Std:  {all_voltages.std():.6f}")
    # print(f"  Min:  {all_voltages.min():.6f}")
    # print(f"  Max:  {all_voltages.max():.6f}")
    
    # # Fit polynomials (1st, 2nd, 3rd order)
    # print(f"\n{'='*80}")
    # print(f"POLYNOMIAL FITTING (DAC vs Voltage)")
    # print(f"{'='*80}")
    # print(f"Fitting polynomials: Voltage = f(DAC)")
    
    # Use DAC values as x and voltages as y for fitting
    x_dac = all_dac_values
    y_voltage = all_voltages
    
    # Fit 1st order (linear)
    poly1_coeffs = np.polyfit(x_dac, y_voltage, 1)
    poly1_fit = np.poly1d(poly1_coeffs)
    poly1_values = poly1_fit(x_dac)
    residuals1 = y_voltage - poly1_values
    r_squared1 = 1 - (np.sum(residuals1**2) / np.sum((y_voltage - y_voltage.mean())**2))
    rmse1 = np.sqrt(np.mean(residuals1**2))
    
    # print(f"\n1st Order Polynomial (Linear):")
    # print(f"  Equation: Voltage = {poly1_coeffs[0]:.10e}*DAC + {poly1_coeffs[1]:.10e}")
    # print(f"  R² = {r_squared1:.10f}")
    # print(f"  RMSE = {rmse1:.10e}")
    
    # Fit 2nd order (quadratic)
    # poly2_coeffs = np.polyfit(x_dac, y_voltage, 2)
    # poly2_fit = np.poly1d(poly2_coeffs)
    # poly2_values = poly2_fit(x_dac)
    # residuals2 = y_voltage - poly2_values
    # r_squared2 = 1 - (np.sum(residuals2**2) / np.sum((y_voltage - y_voltage.mean())**2))
    # rmse2 = np.sqrt(np.mean(residuals2**2))
    
    # print(f"\n2nd Order Polynomial (Quadratic):")
    # print(f"  Equation: Voltage = {poly2_coeffs[0]:.10e}*DAC² + {poly2_coeffs[1]:.10e}*DAC + {poly2_coeffs[2]:.10e}")
    # print(f"  R² = {r_squared2:.10f}")
    # print(f"  RMSE = {rmse2:.10e}")
    
    # # Fit 3rd order (cubic)
    # poly3_coeffs = np.polyfit(x_dac, y_voltage, 3)
    # poly3_fit = np.poly1d(poly3_coeffs)
    # poly3_values = poly3_fit(x_dac)
    # residuals3 = y_voltage - poly3_values
    # r_squared3 = 1 - (np.sum(residuals3**2) / np.sum((y_voltage - y_voltage.mean())**2))
    # rmse3 = np.sqrt(np.mean(residuals3**2))
    
    # print(f"\n3rd Order Polynomial (Cubic):")
    # print(f"  Equation: Voltage = {poly3_coeffs[0]:.10e}*DAC³ + {poly3_coeffs[1]:.10e}*DAC² + {poly3_coeffs[2]:.10e}*DAC + {poly3_coeffs[3]:.10e}")
    # print(f"  R² = {r_squared3:.10f}")
    # print(f"  RMSE = {rmse3:.10e}")
    
    # print(f"\n{'='*80}")
    # print(f"MODEL COMPARISON")
    # print(f"{'='*80}")
    # print(f"{'Model':<20} {'R²':<20} {'RMSE':<20}")
    # print(f"{'-'*60}")
    # print(f"{'Linear (1st order)':<20} {r_squared1:<20.10f} {rmse1:<20.10e}")
    # print(f"{'Quadratic (2nd order)':<20} {r_squared2:<20.10f} {rmse2:<20.10e}")
    # print(f"{'Cubic (3rd order)':<20} {r_squared3:<20.10f} {rmse3:<20.10e}")
    
    if plotting:
        # Create detailed plots
        fig = plt.figure(figsize=(16, 12))
        
        # Plot 1: Original filtered data (Voltage vs Time)
        ax1 = plt.subplot(3, 2, 1)
        ax1.plot(filtered_df['timestamp'], filtered_df['voltage'], 'b-', linewidth=0.5, alpha=0.5)
        ax1.set_xlabel('Timestamp (s)')
        ax1.set_ylabel('Voltage (V)')
        ax1.set_title(f'Voltage vs Time [{time_threshold_lower:.3f}, {time_threshold_upper:.3f}]')
        ax1.grid(True, alpha=0.3)
        
        # Plot 2: DAC vs Voltage (measured data)
        ax2 = plt.subplot(3, 2, 2)
        ax2.plot(filtered_df['dac_value'], filtered_df['voltage'], 'k.', markersize=0.5, alpha=0.3, label='Measured Data')
        ax2.set_xlabel('DAC Value')
        ax2.set_ylabel('Voltage (V)')
        ax2.set_title(f'DAC vs Voltage (Raw Measurements)')
        ax2.grid(True, alpha=0.3)
        ax2.legend()
        
        # Plot 3: All polynomial fits
        ax3 = plt.subplot(3, 2, 3)
        # Subsample for clearer visualization
        sample_step = max(1, len(x_dac) // 10000)
        ax3.plot(x_dac[::sample_step], y_voltage[::sample_step], 'k.', markersize=1, alpha=0.3, label='Averaged Data')
        ax3.plot(x_dac[::sample_step], poly1_values[::sample_step], 'r-', linewidth=1.5, label=f'1st Order (R²={r_squared1:.6f})')
        ax3.plot(x_dac[::sample_step], poly2_values[::sample_step], 'g-', linewidth=1.5, label=f'2nd Order (R²={r_squared2:.6f})')
        ax3.plot(x_dac[::sample_step], poly3_values[::sample_step], 'b-', linewidth=1.5, label=f'3rd Order (R²={r_squared3:.6f})')
        ax3.set_xlabel('DAC Value')
        ax3.set_ylabel('Voltage (V)')
        ax3.set_title('Polynomial Fits Comparison')
        ax3.grid(True, alpha=0.3)
        ax3.legend()
        
        # Plot 4: Residuals for 1st order
        ax4 = plt.subplot(3, 2, 4)
        ax4.plot(x_dac[::sample_step], residuals1[::sample_step], 'r.', markersize=0.5, alpha=0.5)
        ax4.axhline(y=0, color='k', linestyle='--', linewidth=1)
        ax4.set_xlabel('DAC Value')
        ax4.set_ylabel('Residuals (V)')
        ax4.set_title(f'1st Order Residuals (RMSE={rmse1:.4e})')
        ax4.grid(True, alpha=0.3)
        
        # Plot 5: Residuals for 2nd order
        ax5 = plt.subplot(3, 2, 5)
        ax5.plot(x_dac[::sample_step], residuals2[::sample_step], 'g.', markersize=0.5, alpha=0.5)
        ax5.axhline(y=0, color='k', linestyle='--', linewidth=1)
        ax5.set_xlabel('DAC Value')
        ax5.set_ylabel('Residuals (V)')
        ax5.set_title(f'2nd Order Residuals (RMSE={rmse2:.4e})')
        ax5.grid(True, alpha=0.3)
        
        # Plot 6: Residuals for 3rd order
        ax6 = plt.subplot(3, 2, 6)
        ax6.plot(x_dac[::sample_step], residuals3[::sample_step], 'b.', markersize=0.5, alpha=0.5)
        ax6.axhline(y=0, color='k', linestyle='--', linewidth=1)
        ax6.set_xlabel('DAC Value')
        ax6.set_ylabel('Residuals (V)')
        ax6.set_title(f'3rd Order Residuals (RMSE={rmse3:.4e})')
        ax6.grid(True, alpha=0.3)
        
        plt.tight_layout()
        plt.show()
        
        # Additional detailed residual analysis plot
        fig2 = plt.figure(figsize=(16, 5))
        
        # Histogram of residuals
        ax1 = plt.subplot(1, 3, 1)
        ax1.hist(residuals1, bins=100, alpha=0.5, color='r', label='1st Order', density=True)
        ax1.hist(residuals2, bins=100, alpha=0.5, color='g', label='2nd Order', density=True)
        ax1.hist(residuals3, bins=100, alpha=0.5, color='b', label='3rd Order', density=True)
        ax1.set_xlabel('Residual Value')
        ax1.set_ylabel('Density')
        ax1.set_title('Residual Distributions')
        ax1.legend()
        ax1.grid(True, alpha=0.3)
        
        # Q-Q plot for 3rd order (best fit)
        ax2 = plt.subplot(1, 3, 2)
        stats.probplot(residuals3, dist="norm", plot=ax2)
        ax2.set_title('Q-Q Plot (3rd Order Residuals)')
        ax2.grid(True, alpha=0.3)
        
        # Comparison of fits in voltage space
        ax3 = plt.subplot(1, 3, 3)
        sample_indices = np.linspace(0, len(x_dac)-1, 1000, dtype=int)
        ax3.scatter(y_voltage[sample_indices], poly1_values[sample_indices], 
                    c='r', s=1, alpha=0.3, label='1st Order')
        ax3.scatter(y_voltage[sample_indices], poly2_values[sample_indices], 
                    c='g', s=1, alpha=0.3, label='2nd Order')
        ax3.scatter(y_voltage[sample_indices], poly3_values[sample_indices], 
                    c='b', s=1, alpha=0.3, label='3rd Order')
        min_val = min(y_voltage.min(), poly1_values.min(), poly2_values.min(), poly3_values.min())
        max_val = max(y_voltage.max(), poly1_values.max(), poly2_values.max(), poly3_values.max())
        ax3.plot([min_val, max_val], [min_val, max_val], 'k--', linewidth=1, label='Perfect Fit')
        ax3.set_xlabel('Actual Voltage (V)')
        ax3.set_ylabel('Predicted Voltage (V)')
        ax3.set_title('Predicted vs Actual (Sample)')
        ax3.legend()
        ax3.grid(True, alpha=0.3)
        
        plt.tight_layout()
        plt.show()
    
    # print(f"\n{'='*80}")
    # print("Analysis complete!")
    # print(f"{'='*80}\n")
    
    return {
        'dac_values': all_dac_values,
        'voltages': all_voltages,
        'grouped_data': grouped,
        'poly1': (poly1_coeffs, r_squared1, rmse1)#,
        # 'poly2': (poly2_coeffs, r_squared2, rmse2),
        # 'poly3': (poly3_coeffs, r_squared3, rmse3)
    }


if __name__ == "__main__":
    # Example usage - edit these parameters as needed:
    # analyze_and_fit_polynomials(time_threshold_lower=0.0, time_threshold_upper=0.175, dac_range=65536)
    
    # arr = create_voltage_sweep(65536*2, "x")
    # plot_voltage_vs_time()
    
    # Run analysis with default parameters
    start_time_threshold = 0.421
    inc = 0.351
    iterations = 23

    poly1_coeffs = []
    
    for i in range(iterations):
        time_threshold_lower = start_time_threshold + i * inc
        time_threshold_upper = time_threshold_lower + inc
        results = analyze_and_fit_polynomials(time_threshold_lower, time_threshold_upper)
        poly1_coeffs.append(results['poly1'][0])


    poly1_coeffs = np.array(poly1_coeffs)
    for i in range(poly1_coeffs.shape[0]):
        if poly1_coeffs[i][0] < 0:
            poly1_coeffs[i][0] *= -1
            poly1_coeffs[i][1] *= -1
    print(poly1_coeffs)
    print(np.mean(poly1_coeffs, axis=0))