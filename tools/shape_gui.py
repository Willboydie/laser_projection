import tkinter as tk
from tkinter import ttk, messagebox, filedialog
import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure
from shapes import cartesianCircle, cartesianRectangle, hemiRectangle
import sys
import os

# Add paths for imports
sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'spherical'))
sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'save'))
from spherical import toGalvoSpace, toSpherical
from save import createBinFile, saveArray


class Shape3DGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("3D Shape Path Generator")
        self.root.geometry("1200x800")
        
        # Variables for shape parameters
        self.shape_var = tk.StringVar(value="Circle")
        self.center_x = tk.DoubleVar(value=1.0)
        self.center_y = tk.DoubleVar(value=0.0)
        self.center_z = tk.DoubleVar(value=0.0)
        self.normal_x = tk.DoubleVar(value=0.0)
        self.normal_y = tk.DoubleVar(value=0.0)
        self.normal_z = tk.DoubleVar(value=1.0)
        
        # Shape-specific parameters
        self.radius = tk.DoubleVar(value=1.0)
        self.width = tk.DoubleVar(value=2.0)
        self.height = tk.DoubleVar(value=2.0)
        self.increment = tk.DoubleVar(value=0.01)
        
        # On/Off section parameters
        self.fraction_numerator = tk.IntVar(value=1)
        self.fraction_denominator = tk.IntVar(value=2)
        
        # Save parameters
        self.max_angle = tk.DoubleVar(value=0.47)
        self.max_volts = tk.DoubleVar(value=7.5)
        
        # Store current path for saving
        self.current_path = None
        
        self.setup_ui()
        self.update_parameter_visibility()
        
    def setup_ui(self):
        # Main frame
        main_frame = ttk.Frame(self.root)
        main_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        
        # Left panel for controls
        control_frame = ttk.LabelFrame(main_frame, text="Shape Parameters", padding=10)
        control_frame.pack(side=tk.LEFT, fill=tk.Y, padx=(0, 10))
        
        # Shape selection
        ttk.Label(control_frame, text="Shape:").grid(row=0, column=0, sticky=tk.W, pady=5)
        shape_combo = ttk.Combobox(control_frame, textvariable=self.shape_var, 
                                 values=["Circle", "Rectangle", "Hemi-Rectangle"],
                                 state="readonly", width=15)
        shape_combo.grid(row=0, column=1, sticky=tk.W, pady=5)
        shape_combo.bind('<<ComboboxSelected>>', self.on_shape_change)
        
        # Center point
        ttk.Label(control_frame, text="Center Point:").grid(row=1, column=0, columnspan=2, sticky=tk.W, pady=(20, 5))
        
        ttk.Label(control_frame, text="X:").grid(row=2, column=0, sticky=tk.W, padx=(20, 5))
        ttk.Entry(control_frame, textvariable=self.center_x, width=10).grid(row=2, column=1, sticky=tk.W)
        
        ttk.Label(control_frame, text="Y:").grid(row=3, column=0, sticky=tk.W, padx=(20, 5))
        ttk.Entry(control_frame, textvariable=self.center_y, width=10).grid(row=3, column=1, sticky=tk.W)
        
        ttk.Label(control_frame, text="Z:").grid(row=4, column=0, sticky=tk.W, padx=(20, 5))
        ttk.Entry(control_frame, textvariable=self.center_z, width=10).grid(row=4, column=1, sticky=tk.W)
        
        # Normal vector
        ttk.Label(control_frame, text="Normal Vector:").grid(row=5, column=0, columnspan=2, sticky=tk.W, pady=(20, 5))
        
        ttk.Label(control_frame, text="X:").grid(row=6, column=0, sticky=tk.W, padx=(20, 5))
        ttk.Entry(control_frame, textvariable=self.normal_x, width=10).grid(row=6, column=1, sticky=tk.W)
        
        ttk.Label(control_frame, text="Y:").grid(row=7, column=0, sticky=tk.W, padx=(20, 5))
        ttk.Entry(control_frame, textvariable=self.normal_y, width=10).grid(row=7, column=1, sticky=tk.W)
        
        ttk.Label(control_frame, text="Z:").grid(row=8, column=0, sticky=tk.W, padx=(20, 5))
        ttk.Entry(control_frame, textvariable=self.normal_z, width=10).grid(row=8, column=1, sticky=tk.W)
        
        # Shape-specific parameters
        self.params_frame = ttk.LabelFrame(control_frame, text="Shape Parameters", padding=5)
        self.params_frame.grid(row=9, column=0, columnspan=2, sticky=tk.W+tk.E, pady=(20, 5))
        
        # Initialize parameter widgets (will be created dynamically)
        self.radius_label = None
        self.radius_entry = None
        self.width_label = None
        self.width_entry = None
        self.height_label = None
        self.height_entry = None
        
        # Common parameters (always visible)
        ttk.Label(self.params_frame, text="Increment:").grid(row=0, column=0, sticky=tk.W, pady=5)
        self.increment_entry = ttk.Entry(self.params_frame, textvariable=self.increment, width=10)
        self.increment_entry.grid(row=0, column=1, sticky=tk.W, pady=5)
        
        # On/Off section parameters (always visible)
        ttk.Label(self.params_frame, text="Section Fraction:").grid(row=1, column=0, sticky=tk.W, pady=5)
        self.fraction_frame = ttk.Frame(self.params_frame)
        self.fraction_frame.grid(row=1, column=1, sticky=tk.W, pady=5)
        self.fraction_num_entry = ttk.Entry(self.fraction_frame, textvariable=self.fraction_numerator, width=5)
        self.fraction_num_entry.pack(side=tk.LEFT)
        ttk.Label(self.fraction_frame, text="/").pack(side=tk.LEFT, padx=2)
        self.fraction_den_entry = ttk.Entry(self.fraction_frame, textvariable=self.fraction_denominator, width=5)
        self.fraction_den_entry.pack(side=tk.LEFT)
        
        # Save parameters (always visible)
        ttk.Label(self.params_frame, text="Max Angle:").grid(row=2, column=0, sticky=tk.W, pady=5)
        ttk.Entry(self.params_frame, textvariable=self.max_angle, width=10).grid(row=2, column=1, sticky=tk.W, pady=5)
        
        ttk.Label(self.params_frame, text="Max Volts:").grid(row=3, column=0, sticky=tk.W, pady=5)
        ttk.Entry(self.params_frame, textvariable=self.max_volts, width=10).grid(row=3, column=1, sticky=tk.W, pady=5)
        
        # Generate and Save buttons
        button_frame = ttk.Frame(control_frame)
        button_frame.grid(row=10, column=0, columnspan=2, pady=20)
        
        ttk.Button(button_frame, text="Generate Shape", command=self.generate_shape).pack(side=tk.LEFT, padx=5)
        ttk.Button(button_frame, text="Save Path", command=self.save_path).pack(side=tk.LEFT, padx=5)
        ttk.Button(button_frame, text="Save Array", command=self.save_array).pack(side=tk.LEFT, padx=5)
        
        # Right panel for 3D visualization
        viz_frame = ttk.LabelFrame(main_frame, text="3D Visualization", padding=10)
        viz_frame.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True)
        
        # Create matplotlib figure with square aspect ratio
        self.fig = Figure(figsize=(8, 8), dpi=100)
        self.ax = self.fig.add_subplot(111, projection='3d')
        self.ax.set_xlabel('X')
        self.ax.set_ylabel('Y')
        self.ax.set_zlabel('Z')
        self.ax.set_title('3D Shape Path')
        
        # Embed matplotlib in tkinter
        self.canvas = FigureCanvasTkAgg(self.fig, viz_frame)
        self.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)
        
    def on_shape_change(self, event=None):
        """Update parameter visibility based on selected shape"""
        self.update_parameter_visibility()
        
    def update_parameter_visibility(self):
        """Show/hide parameters based on selected shape"""
        # Clear existing shape-specific parameter widgets (preserve common ones)
        for widget in self.params_frame.winfo_children():
            widget_text = widget.cget('text') if widget.winfo_class() == 'TLabel' else ''
            if widget_text in ['Radius:', 'Width:', 'Height:']:
                widget.destroy()
        
        # Reset widget references
        self.radius_label = None
        self.radius_entry = None
        self.width_label = None
        self.width_entry = None
        self.height_label = None
        self.height_entry = None
        
        shape = self.shape_var.get()
        
        if shape == "Circle":
            self.radius_label = ttk.Label(self.params_frame, text="Radius:")
            self.radius_label.grid(row=4, column=0, sticky=tk.W, pady=5)
            self.radius_entry = ttk.Entry(self.params_frame, textvariable=self.radius, width=10)
            self.radius_entry.grid(row=4, column=1, sticky=tk.W, pady=5)
        elif shape in ["Rectangle", "Hemi-Rectangle"]:
            self.width_label = ttk.Label(self.params_frame, text="Width:")
            self.width_label.grid(row=4, column=0, sticky=tk.W, pady=5)
            self.width_entry = ttk.Entry(self.params_frame, textvariable=self.width, width=10)
            self.width_entry.grid(row=4, column=1, sticky=tk.W, pady=5)
            self.height_label = ttk.Label(self.params_frame, text="Height:")
            self.height_label.grid(row=5, column=0, sticky=tk.W, pady=5)
            self.height_entry = ttk.Entry(self.params_frame, textvariable=self.height, width=10)
            self.height_entry.grid(row=5, column=1, sticky=tk.W, pady=5)
    
    def generate_shape(self):
        """Generate and display the 3D shape"""
        try:
            # Get parameters
            center = np.array([self.center_x.get(), self.center_y.get(), self.center_z.get()])
            normal = np.array([self.normal_x.get(), self.normal_y.get(), self.normal_z.get()])
            inc = self.increment.get()
            
            # Normalize normal vector
            if np.linalg.norm(normal) == 0:
                normal = np.array([0, 0, 1])
            else:
                normal = normal / np.linalg.norm(normal)
            
            # Generate shape based on selection
            shape = self.shape_var.get()
            
            if shape == "Circle":
                radius = self.radius.get()
                path = cartesianCircle(radius, inc, 0)  # Generate in XY plane first
            elif shape == "Rectangle":
                width = self.width.get()
                height = self.height.get()
                path = self.create_rectangle(inc, width, height)
            elif shape == "Hemi-Rectangle":
                width = self.width.get()
                height = self.height.get()
                path = self.create_hemi_rectangle(inc, width, height)
            
            # Transform path to desired center and orientation
            transformed_path = self.transform_path(path, center, normal)
            
            # Add 4th dimension with on/off pattern
            path_with_states = self.add_on_off_states(transformed_path)
            
            # Store current path for saving
            self.current_path = path_with_states
            
            # Clear previous plot
            self.ax.clear()
            
            # Plot the path with color-coded segments
            self.plot_segmented_path(path_with_states)
            
            # Add start and end markers
            self.ax.scatter(transformed_path[0, 0], transformed_path[0, 1], transformed_path[0, 2], 
                          color='red', s=20, label='Start')
            self.ax.scatter(transformed_path[-1, 0], transformed_path[-1, 1], transformed_path[-1, 2], 
                          color='blue', s=20, label='End')
            
            # Set labels and title
            self.ax.set_xlabel('X')
            self.ax.set_ylabel('Y')
            self.ax.set_zlabel('Z')
            self.ax.set_title(f'3D {shape} Path')
            self.ax.legend()
            
            # Set equal axis scales to maintain true shape proportions
            self.set_equal_axis_scales()
            
            # Refresh canvas
            self.canvas.draw()
            
        except Exception as e:
            messagebox.showerror("Error", f"Error generating shape: {str(e)}")
    
    def transform_path(self, path, center, normal):
        """Transform path to desired center and orientation"""
        # Create rotation matrix to align with normal vector
        # Default orientation is along Z-axis
        default_normal = np.array([0, 0, 1])
        
        # If normal is already along Z-axis, no rotation needed
        if np.allclose(normal, default_normal):
            rotation_matrix = np.eye(3)
        elif np.allclose(normal, -default_normal):
            # 180-degree rotation around X-axis
            rotation_matrix = np.array([[-1, 0, 0], [0, -1, 0], [0, 0, 1]])
        else:
            # Calculate rotation axis and angle
            rotation_axis = np.cross(default_normal, normal)
            rotation_axis = rotation_axis / np.linalg.norm(rotation_axis)
            
            cos_angle = np.dot(default_normal, normal)
            sin_angle = np.linalg.norm(np.cross(default_normal, normal))
            
            # Rodrigues' rotation formula
            K = np.array([[0, -rotation_axis[2], rotation_axis[1]],
                         [rotation_axis[2], 0, -rotation_axis[0]],
                         [-rotation_axis[1], rotation_axis[0], 0]])
            
            rotation_matrix = np.eye(3) + sin_angle * K + (1 - cos_angle) * np.dot(K, K)
        
        # Apply rotation and translation
        rotated_path = np.dot(path, rotation_matrix.T)
        transformed_path = rotated_path + center
        
        return transformed_path
    
    def create_rectangle(self, inc, width, height):
        """Create a rectangle with specified width and height"""
        half_width = width / 2
        half_height = height / 2
        
        # Top side (left to right)
        y_top = np.arange(-half_width, half_width, inc)
        z_top = np.full_like(y_top, half_height)
        x_top = np.zeros_like(y_top)
        top = np.column_stack((x_top, y_top, z_top))
        
        # Right side (top to bottom)
        z_right = np.arange(half_height, -half_height, -inc)
        y_right = np.full_like(z_right, half_width)
        x_right = np.zeros_like(z_right)
        right = np.column_stack((x_right, y_right, z_right))
        
        # Bottom side (right to left)
        y_bottom = np.arange(half_width, -half_width, -inc)
        z_bottom = np.full_like(y_bottom, -half_height)
        x_bottom = np.zeros_like(y_bottom)
        bottom = np.column_stack((x_bottom, y_bottom, z_bottom))
        
        # Left side (bottom to top)
        z_left = np.arange(-half_height, half_height, inc)
        y_left = np.full_like(z_left, -half_width)
        x_left = np.zeros_like(z_left)
        left = np.column_stack((x_left, y_left, z_left))
        
        # Concatenate in chronological order: top → right → bottom → left
        path = np.concatenate((top, right, bottom, left), axis=0)
        return path
    
    def create_hemi_rectangle(self, inc, width, height):
        """Create a hemi-rectangle with specified width and height"""
        half_width = width / 2
        half_height = height / 2
        
        # Bottom side (right to left)
        y_bottom = np.arange(half_width, -half_width, -inc)
        z_bottom = np.full_like(y_bottom, -half_height)
        x_bottom = np.zeros_like(y_bottom)
        bottom = np.column_stack((x_bottom, y_bottom, z_bottom))
        
        # Left side (bottom to center)
        z_square_left = np.arange(-half_height, 0, inc)
        y_square_left = np.full_like(z_square_left, -half_width)
        x_square_left = np.zeros_like(z_square_left)
        left_square = np.column_stack((x_square_left, y_square_left, z_square_left))
        
        # Right side (center to bottom)
        z_square_right = np.arange(0, -half_height, -inc)
        y_square_right = np.full_like(z_square_right, half_width)
        x_square_right = np.zeros_like(z_square_right)
        right_square = np.column_stack((x_square_right, y_square_right, z_square_right))
        
        # Top semicircle
        t = np.linspace(-np.pi/2, np.pi/2, int(np.pi * half_width / inc))
        z = half_height * np.cos(t)
        y = half_width * np.sin(t)
        x = np.zeros_like(z)
        top_hemi = np.column_stack((x, y, z))
        
        # Concatenate in chronological order: top → right → bottom → left
        path = np.concatenate((top_hemi, right_square, bottom, left_square), axis=0)
        return path
    
    def add_on_off_states(self, path):
        """Add 4th dimension with alternating on/off states based on fraction"""
        num_points = len(path)
        numerator = self.fraction_numerator.get()
        denominator = self.fraction_denominator.get()
        
        # Calculate section size
        section_size = num_points // denominator
        remainder = num_points % denominator
        
        # Create 4th dimension array
        states = np.zeros(num_points)
        
        # Distribute remainder points across first sections
        current_point = 0
        for i in range(denominator):
            # Calculate section length (add 1 to first 'remainder' sections)
            section_length = section_size + (1 if i < remainder else 0)
            
            # Determine if this section should be "on" (1) or "off" (0)
            # Start with "on" (1) for first section, then alternate
            state = 1 if i % 2 == 0 else 0
            
            # Set states for this section
            end_point = current_point + section_length
            states[current_point:end_point] = state
            current_point = end_point
        
        # Add 4th dimension to path
        path_with_states = np.column_stack((path, states))
        return path_with_states
    
    def plot_segmented_path(self, path_with_states):
        """Plot path with color-coded segments based on on/off states"""
        x = path_with_states[:, 0]
        y = path_with_states[:, 1]
        z = path_with_states[:, 2]
        states = path_with_states[:, 3]
        
        # Find segment boundaries
        segment_starts = np.where(np.diff(states) != 0)[0] + 1
        segment_starts = np.concatenate(([0], segment_starts, [len(states)]))
        
        # Plot each segment with appropriate color
        for i in range(len(segment_starts) - 1):
            start_idx = segment_starts[i]
            end_idx = segment_starts[i + 1]
            
            # Determine color based on state
            state = states[start_idx]
            color = 'green' if state == 1 else 'black'
            linewidth = 2 if state == 1 else 1
            
            # Plot this segment (inclusive of both start and end points to avoid gaps)
            segment_x = x[start_idx:end_idx]
            segment_y = y[start_idx:end_idx]
            segment_z = z[start_idx:end_idx]
            
            self.ax.plot(segment_x, segment_y, segment_z, 
                        color=color, linewidth=linewidth)
    
    def set_equal_axis_scales(self):
        """Set equal axis scales to maintain true shape proportions"""
        try:
            # Get current axis limits
            x_limits = self.ax.get_xlim()
            y_limits = self.ax.get_ylim()
            z_limits = self.ax.get_zlim()
            
            # Calculate the range for each axis
            x_range = x_limits[1] - x_limits[0]
            y_range = y_limits[1] - y_limits[0]
            z_range = z_limits[1] - z_limits[0]
            
            # Find the maximum range to use for all axes
            max_range = max(x_range, y_range, z_range)
            
            # Calculate centers for each axis
            x_center = (x_limits[0] + x_limits[1]) / 2
            y_center = (y_limits[0] + y_limits[1]) / 2
            z_center = (z_limits[0] + z_limits[1]) / 2
            
            # Set equal ranges around the centers
            half_range = max_range / 2
            self.ax.set_xlim(x_center - half_range, x_center + half_range)
            self.ax.set_ylim(y_center - half_range, y_center + half_range)
            self.ax.set_zlim(z_center - half_range, z_center + half_range)
            
        except Exception as e:
            # Fallback: use set_box_aspect for equal scaling
            self.ax.set_box_aspect([1, 1, 1])
    
    def save_path(self):
        """Save the current path to a binary file"""
        if self.current_path is None:
            messagebox.showwarning("No Path", "Please generate a shape first before saving.")
            return
        
        try:
            # Get save parameters
            max_angle = self.max_angle.get()
            max_volts = self.max_volts.get()
            
            # Convert cartesian path to spherical coordinates
            angle_data = []
            for point in self.current_path:
                galvo_point = toGalvoSpace(point[0], point[1], point[2], point[3])
                # galvo_point = toSpherical(point)
                angle_data.append(galvo_point)
            
            angle_data = np.array(angle_data)
            
            # Ask user for save location
            file_path = filedialog.asksaveasfilename(
                defaultextension=".bin",
                filetypes=[("Binary files", "*.bin"), ("All files", "*.*")],
                title="Save Path File"
            )
            
            if file_path:
                # Create directory if it doesn't exist
                os.makedirs(os.path.dirname(file_path), exist_ok=True)
                
                # Save the file using createBinFile
                bin_data = createBinFile(angle_data, max_angle, max_volts)
                
                # Write to user-specified location
                with open(file_path, 'wb') as f:
                    f.write(bin_data)
                
                messagebox.showinfo("Save Successful", 
                                 f"Path saved to:\n{file_path}\n\n"
                                 f"Points: {len(angle_data)}\n"
                                 f"Max Angle: {max_angle}\n"
                                 f"Max Volts: {max_volts}")
                
        except Exception as e:
            messagebox.showerror("Save Error", f"Error saving path: {str(e)}")
    
    def save_array(self):
        """Save the current path as a numpy array"""
        if self.current_path is None:
            messagebox.showwarning("No Path", "Please generate a shape first before saving.")
            return
        
        try:
            # Ask user for filename
            filename = filedialog.asksaveasfilename(
                defaultextension=".npy",
                filetypes=[("NumPy files", "*.npy"), ("All files", "*.*")],
                title="Save Array File",
                initialdir=os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))), 'data', 'arrays')
            )
            
            if filename:
                # Extract just the filename without path
                filename_only = os.path.basename(filename)
                
                # Convert cartesian path to galvo angles
                # toGalvoSpace returns [a, b, laser_on] when given arrays, so we need to transpose
                galvo_path = toGalvoSpace(
                    self.current_path[:, 0], 
                    self.current_path[:, 1], 
                    self.current_path[:, 2], 
                    self.current_path[:, 3]
                )
                
                # Save using saveArray function
                saved_path = saveArray(galvo_path, filename_only)
                
                messagebox.showinfo("Save Successful", 
                                 f"Array saved to:\n{saved_path}\n\n"
                                 f"Shape: {galvo_path.shape}\n"
                                 f"Points: {len(galvo_path)}")
                
        except Exception as e:
            messagebox.showerror("Save Error", f"Error saving array: {str(e)}")


def main():
    root = tk.Tk()
    app = Shape3DGUI(root)
    root.mainloop()


if __name__ == "__main__":
    main()
