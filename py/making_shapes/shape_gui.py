import tkinter as tk
from tkinter import ttk, messagebox
import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure
from shapes import cartesianCircle, cartesianRectangle, hemiRectangle


class Shape3DGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("3D Shape Path Generator")
        self.root.geometry("1200x800")
        
        # Variables for shape parameters
        self.shape_var = tk.StringVar(value="Circle")
        self.center_x = tk.DoubleVar(value=0.0)
        self.center_y = tk.DoubleVar(value=0.0)
        self.center_z = tk.DoubleVar(value=0.0)
        self.normal_x = tk.DoubleVar(value=0.0)
        self.normal_y = tk.DoubleVar(value=0.0)
        self.normal_z = tk.DoubleVar(value=1.0)
        
        # Shape-specific parameters
        self.radius = tk.DoubleVar(value=1.0)
        self.width = tk.DoubleVar(value=2.0)
        self.height = tk.DoubleVar(value=2.0)
        self.increment = tk.DoubleVar(value=0.1)
        
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
        
        # Common parameters
        ttk.Label(self.params_frame, text="Increment:").grid(row=0, column=0, sticky=tk.W, pady=5)
        ttk.Entry(self.params_frame, textvariable=self.increment, width=10).grid(row=0, column=1, sticky=tk.W, pady=5)
        
        # Generate button
        ttk.Button(control_frame, text="Generate Shape", command=self.generate_shape).grid(row=10, column=0, columnspan=2, pady=20)
        
        # Right panel for 3D visualization
        viz_frame = ttk.LabelFrame(main_frame, text="3D Visualization", padding=10)
        viz_frame.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True)
        
        # Create matplotlib figure
        self.fig = Figure(figsize=(8, 6), dpi=100)
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
        # Clear existing parameter widgets (except increment)
        for widget in self.params_frame.winfo_children():
            if widget.winfo_class() != 'TLabel' or 'Increment' not in widget.cget('text'):
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
            self.radius_label.grid(row=1, column=0, sticky=tk.W, pady=5)
            self.radius_entry = ttk.Entry(self.params_frame, textvariable=self.radius, width=10)
            self.radius_entry.grid(row=1, column=1, sticky=tk.W, pady=5)
        elif shape in ["Rectangle", "Hemi-Rectangle"]:
            self.width_label = ttk.Label(self.params_frame, text="Width:")
            self.width_label.grid(row=1, column=0, sticky=tk.W, pady=5)
            self.width_entry = ttk.Entry(self.params_frame, textvariable=self.width, width=10)
            self.width_entry.grid(row=1, column=1, sticky=tk.W, pady=5)
            self.height_label = ttk.Label(self.params_frame, text="Height:")
            self.height_label.grid(row=2, column=0, sticky=tk.W, pady=5)
            self.height_entry = ttk.Entry(self.params_frame, textvariable=self.height, width=10)
            self.height_entry.grid(row=2, column=1, sticky=tk.W, pady=5)
    
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
                r = min(self.width.get(), self.height.get()) / 2
                path = cartesianRectangle(inc, 0, r)  # Generate in YZ plane first
            elif shape == "Hemi-Rectangle":
                r = min(self.width.get(), self.height.get()) / 2
                path = hemiRectangle(inc, 0, r)  # Generate in YZ plane first
            
            # Transform path to desired center and orientation
            transformed_path = self.transform_path(path, center, normal)
            
            # Clear previous plot
            self.ax.clear()
            
            # Plot the path
            self.ax.plot(transformed_path[:, 0], transformed_path[:, 1], transformed_path[:, 2], 'b-', linewidth=2)
            self.ax.scatter(transformed_path[0, 0], transformed_path[0, 1], transformed_path[0, 2], 
                          color='red', s=50, label='Start')
            self.ax.scatter(transformed_path[-1, 0], transformed_path[-1, 1], transformed_path[-1, 2], 
                          color='green', s=50, label='End')
            
            # Set labels and title
            self.ax.set_xlabel('X')
            self.ax.set_ylabel('Y')
            self.ax.set_zlabel('Z')
            self.ax.set_title(f'3D {shape} Path')
            self.ax.legend()
            
            # Set equal aspect ratio
            self.ax.set_box_aspect([1,1,1])
            
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


def main():
    root = tk.Tk()
    app = Shape3DGUI(root)
    root.mainloop()


if __name__ == "__main__":
    main()
