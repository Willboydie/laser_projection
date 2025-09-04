import tkinter as tk
from tkinter import ttk, filedialog, messagebox
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure
import numpy as np
import os
from dxf_processing import extract_layers, read_dxf_vectors, save_dxf

class DXFViewerGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("DXF Layer Viewer")
        self.root.geometry("1400x1000")
        
        # Data storage
        self.dxf_file_path = None
        self.layers_data = None
        self.vectors_data = None
        self.selected_layers = set()
        
        # Create GUI components
        self.create_widgets()
        
    def create_widgets(self):
        # Main frame
        main_frame = ttk.Frame(self.root, padding="3")
        main_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Configure grid weights for side-by-side layout
        self.root.columnconfigure(0, weight=1)
        self.root.rowconfigure(0, weight=1)
        main_frame.columnconfigure(1, weight=3)  # Plot area gets more horizontal space
        main_frame.columnconfigure(0, weight=1)  # Controls get less space
        
        # Configure row heights - make first row much shorter
        main_frame.rowconfigure(0, weight=0)  # Minimal height for header
        main_frame.rowconfigure(1, weight=0)  # Minimal height for file selection
        main_frame.rowconfigure(2, weight=0)  # Minimal height for processing options header
        main_frame.rowconfigure(3, weight=0)  # Minimal height for checkboxes
        main_frame.rowconfigure(4, weight=0)  # Minimal height for epsilon
        main_frame.rowconfigure(5, weight=0)  # Minimal height for process button
        main_frame.rowconfigure(6, weight=0)  # Minimal height for layer selection header
        main_frame.rowconfigure(7, weight=2)  # More height for layer listbox
        main_frame.rowconfigure(8, weight=0)  # Minimal height for plot button
        main_frame.rowconfigure(9, weight=0)  # Minimal height for save button
        main_frame.rowconfigure(10, weight=0)  # Minimal height for status
        
        # File selection section
        ttk.Label(main_frame, text="DXF File Selection", font=("Arial", 12, "bold")).grid(row=0, column=0, sticky=tk.W, pady=(0, 2))
        
        ttk.Button(main_frame, text="Browse DXF File", command=self.browse_file).grid(row=1, column=0, sticky=tk.W, padx=(0, 2))
        self.file_label = ttk.Label(main_frame, text="No file selected")
        self.file_label.grid(row=1, column=0, sticky=tk.E)
        
        # Processing options
        ttk.Label(main_frame, text="Processing Options", font=("Arial", 12, "bold")).grid(row=2, column=0, sticky=tk.W, pady=(20, 10))
        
        # Create a frame for checkboxes to keep them compact
        checkbox_frame = ttk.Frame(main_frame)
        checkbox_frame.grid(row=3, column=0, sticky=tk.W, pady=(0, 10))
        
        # Checkboxes for processing sections
        self.msp_var = tk.BooleanVar(value=True)
        self.psp_var = tk.BooleanVar(value=False)
        self.blocks_var = tk.BooleanVar(value=False)
        
        ttk.Checkbutton(checkbox_frame, text="Model Space (MSP)", variable=self.msp_var).grid(row=0, column=0, sticky=tk.W, padx=(0, 20))
        ttk.Checkbutton(checkbox_frame, text="Paper Space (PSP)", variable=self.psp_var).grid(row=0, column=1, sticky=tk.W, padx=(0, 20))
        ttk.Checkbutton(checkbox_frame, text="Blocks", variable=self.blocks_var).grid(row=0, column=2, sticky=tk.W)
        
        # Epsilon value for processing
        eps_frame = ttk.Frame(main_frame)
        eps_frame.grid(row=4, column=0, sticky=tk.W, pady=(10, 0))
        
        ttk.Label(eps_frame, text="Epsilon (tolerance):").grid(row=0, column=0, sticky=tk.W)
        self.eps_var = tk.DoubleVar(value=0.01)
        eps_entry = ttk.Entry(eps_frame, textvariable=self.eps_var, width=10)
        eps_entry.grid(row=0, column=1, sticky=tk.W, padx=(10, 0))
        
        # Process button
        ttk.Button(main_frame, text="Process DXF File", command=self.process_dxf).grid(row=5, column=0, sticky=tk.W, pady=(10, 0))
        
        # Layer selection section
        ttk.Label(main_frame, text="Layer Selection", font=("Arial", 12, "bold")).grid(row=6, column=0, sticky=tk.W, pady=(20, 10))
        
        # Create frame for layer selection
        layer_frame = ttk.Frame(main_frame)
        layer_frame.grid(row=7, column=0, sticky=(tk.W, tk.E), pady=(0, 10))
        
        # Layer listbox with scrollbar
        ttk.Label(layer_frame, text="Available Layers:").grid(row=0, column=0, sticky=tk.W)
        
        listbox_frame = ttk.Frame(layer_frame)
        listbox_frame.grid(row=1, column=0, sticky=(tk.W, tk.E), pady=(5, 0))
        
        self.layer_listbox = tk.Listbox(listbox_frame, height=18, width=18, selectmode=tk.MULTIPLE)
        layer_scrollbar = ttk.Scrollbar(listbox_frame, orient=tk.VERTICAL, command=self.layer_listbox.yview)
        self.layer_listbox.configure(yscrollcommand=layer_scrollbar.set)
        
        self.layer_listbox.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        layer_scrollbar.grid(row=0, column=1, sticky=(tk.N, tk.S))
        
        listbox_frame.columnconfigure(0, weight=1)
        listbox_frame.rowconfigure(0, weight=1)
        
        # Layer info display
        self.layer_info_label = ttk.Label(layer_frame, text="Select layers to view information", wraplength=200)
        self.layer_info_label.grid(row=2, column=0, sticky=tk.W, pady=(5, 0))
        
        # Bind listbox selection event
        self.layer_listbox.bind('<<ListboxSelect>>', self.on_layer_selection)
        
        # Plot button
        ttk.Button(main_frame, text="Plot Selected Layers", command=self.plot_layers).grid(row=8, column=0, sticky=tk.W, pady=(10, 0))
        
        # Save button
        ttk.Button(main_frame, text="Save Selected Layers", command=self.save_selected_layers).grid(row=9, column=0, sticky=tk.W, pady=(10, 0))
        
        # Status label
        self.status_label = ttk.Label(main_frame, text="Ready to load DXF file")
        self.status_label.grid(row=10, column=0, sticky=tk.W, pady=(10, 0))
        
        # Create notebook for tabs
        self.create_notebook(main_frame)
        
    def create_notebook(self, parent):
        # Create notebook widget
        self.notebook = ttk.Notebook(parent)
        self.notebook.grid(row=0, column=1, rowspan=11, sticky=(tk.W, tk.E, tk.N, tk.S), padx=(20, 0))
        
        # Configure plot area to expand
        parent.columnconfigure(1, weight=3)  # Plot gets more horizontal space
        
        # Create main view tab
        self.create_main_view_tab()
        
        # Create detailed analysis tab
        self.create_detailed_analysis_tab()
        
    def create_main_view_tab(self):
        # Main view tab
        main_tab = ttk.Frame(self.notebook)
        self.notebook.add(main_tab, text="Main View")
        
        # Matplotlib figure for main plotting
        self.fig_main = Figure(figsize=(9, 9), dpi=100)
        self.ax_main = self.fig_main.add_subplot(111)
        
        # Create canvas for main view
        self.canvas_main = FigureCanvasTkAgg(self.fig_main, main_tab)
        self.canvas_main.get_tk_widget().pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        
        # Initial plot
        self.ax_main.set_title("DXF Layer Visualization")
        self.ax_main.set_xlabel("X")
        self.ax_main.set_ylabel("Y")
        self.ax_main.grid(True, alpha=0.3)
        self.ax_main.set_aspect('equal')
        
    def create_detailed_analysis_tab(self):
        # Detailed analysis tab
        analysis_tab = ttk.Frame(self.notebook)
        self.notebook.add(analysis_tab, text="Detailed Analysis")
        
        # Create a frame for the button at the top
        button_frame = ttk.Frame(analysis_tab)
        button_frame.pack(side=tk.TOP, fill=tk.X, pady=(10, 5))
        
        # Add button to update detailed analysis
        update_button = ttk.Button(button_frame, text="Update Detailed Analysis", command=self.update_detailed_analysis)
        update_button.pack(pady=5)
        
        # Matplotlib figure for detailed analysis
        self.fig_analysis = Figure(figsize=(9, 9), dpi=100)
        self.ax_analysis = self.fig_analysis.add_subplot(111)
        
        # Create canvas for analysis view
        self.canvas_analysis = FigureCanvasTkAgg(self.fig_analysis, analysis_tab)
        self.canvas_analysis.get_tk_widget().pack(fill=tk.BOTH, expand=True, padx=10, pady=(0, 10))
        
        # Initial plot
        self.ax_analysis.set_title("Detailed Vector Analysis (Select single layer)")
        self.ax_analysis.set_xlabel("X")
        self.ax_analysis.set_ylabel("Y")
        self.ax_analysis.grid(True, alpha=0.3)
        self.ax_analysis.set_aspect('equal')
        
    def browse_file(self):
        """Open file dialog to select DXF file"""
        file_path = filedialog.askopenfilename(
            title="Select DXF File",
            filetypes=[("DXF files", "*.dxf"), ("All files", "*.*")]
        )
        
        if file_path:
            self.dxf_file_path = file_path
            self.file_label.config(text=os.path.basename(file_path))
            self.status_label.config(text=f"File loaded: {os.path.basename(file_path)}")
            
            # Extract layers immediately
            self.extract_layers_from_file()  # Updated method name
            
    def extract_layers_from_file(self):  # Renamed from extract_layers
        """Extract layers from the DXF file"""
        try:
            self.layers_data = extract_layers(
                self.dxf_file_path,
                msp=self.msp_var.get(),
                psp=self.psp_var.get(),
                blocks=self.blocks_var.get()
            )
            
            # Populate layer listbox
            self.populate_layer_listbox()
            
            self.status_label.config(text="Layers extracted successfully")
            
        except Exception as e:
            messagebox.showerror("Error", f"Failed to extract layers: {str(e)}")
            self.status_label.config(text="Error extracting layers")
            
    def populate_layer_listbox(self):
        """Populate the layer listbox with available layers"""
        self.layer_listbox.delete(0, tk.END)
        
        # Get all available layers from different sections
        all_layers = set()
        if self.layers_data['msp']:
            all_layers.update(self.layers_data['msp'].keys())
        if self.layers_data['psp']:
            all_layers.update(self.layers_data['psp'].keys())
        if self.layers_data['blocks']:
            all_layers.update(self.layers_data['blocks'].keys())
            
        # Sort layers alphabetically
        sorted_layers = sorted(all_layers)
        
        for layer in sorted_layers:
            self.layer_listbox.insert(tk.END, layer)
            
    def on_layer_selection(self, event):
        """Handle layer selection in listbox"""
        selection = self.layer_listbox.curselection()
        self.selected_layers = set()
        
        for index in selection:
            layer_name = self.layer_listbox.get(index)
            self.selected_layers.add(layer_name)
            
        # Update layer info display
        self.update_layer_info()
        
    def update_layer_info(self):
        """Update the layer information display"""
        if not self.selected_layers:
            self.layer_info_label.config(text="Select layers to view information")
            return
            
        info_text = f"Selected layers: {', '.join(self.selected_layers)}"
        
        # Add entity type information if available
        if self.layers_data:
            entity_info = []
            for layer in self.selected_layers:
                if layer in self.layers_data['msp']:
                    entity_types = self.layers_data['msp'][layer]
                    entity_info.append(f"{layer}: {', '.join(entity_types)}")
                elif layer in self.layers_data['psp']:
                    entity_types = self.layers_data['psp'][layer]
                    entity_info.append(f"{layer}: {', '.join(entity_types)}")
                elif layer in self.layers_data['blocks']:
                    entity_types = self.layers_data['blocks'][layer]
                    entity_info.append(f"{layer}: {', '.join(entity_types)}")
                    
            if entity_info:
                info_text += f"\nEntity types: {'; '.join(entity_info)}"
                
        self.layer_info_label.config(text=info_text)
        
    def process_dxf(self):
        """Process the DXF file to extract vectors"""
        if not self.dxf_file_path:
            messagebox.showwarning("Warning", "Please select a DXF file first")
            return
            
        try:
            self.status_label.config(text="Processing DXF file...")
            self.root.update()
            
            eps = self.eps_var.get()
            self.vectors_data, num_points = read_dxf_vectors(
                self.dxf_file_path,
                eps,
                msp_=self.msp_var.get(),
                psp_=self.psp_var.get(),
                blocks_=self.blocks_var.get()
            )
            
            self.status_label.config(text=f"DXF processed successfully. {num_points} entities found.")
            
        except Exception as e:
            messagebox.showerror("Error", f"Failed to process DXF file: {str(e)}")
            self.status_label.config(text="Error processing DXF file")
            
    def plot_layers(self):
        """Plot the selected layers"""
        if not self.vectors_data:
            messagebox.showwarning("Warning", "Please process the DXF file first")
            return
            
        if not self.selected_layers:
            messagebox.showwarning("Warning", "Please select at least one layer to plot")
            return
            
        try:
            # Clear previous plot
            self.ax_main.clear()
            
            # Plot each selected layer
            colors = plt.cm.Set3(np.linspace(0, 1, len(self.selected_layers)))
            
            for i, layer_name in enumerate(self.selected_layers):
                if layer_name in self.vectors_data:
                    vectors = self.vectors_data[layer_name]
                    color = colors[i]
                    
                    for vector in vectors:
                        if vector is not None and len(vector) > 0:
                            # Handle different vector formats
                            if len(vector.shape) == 2:  # Array of points
                                if vector.shape[1] >= 2:  # At least 2D
                                    x_coords = vector[:, 0]
                                    y_coords = vector[:, 1]
                                    self.ax_main.plot(x_coords, y_coords, color=color, linewidth=1, label=layer_name)
                            elif len(vector.shape) == 1:  # Single point
                                if len(vector) >= 2:
                                    self.ax_main.plot(vector[0], vector[1], 'o', color=color, markersize=3)
                                    
            # Set plot properties
            self.ax_main.set_title(f"DXF Layers: {', '.join(self.selected_layers)}")
            self.ax_main.set_xlabel("X")
            self.ax_main.set_ylabel("Y")
            self.ax_main.grid(True, alpha=0.3)
            self.ax_main.set_aspect('equal')
            
            # Add legend if multiple layers
            if len(self.selected_layers) > 1:
                # Remove duplicate labels
                handles, labels = self.ax_main.get_legend_handles_labels()
                by_label = dict(zip(labels, handles))
                self.ax_main.legend(by_label.values(), by_label.keys(), loc='upper right')
            
            # Update canvas
            self.canvas_main.draw()
            
            self.status_label.config(text=f"Plotted {len(self.selected_layers)} layers")
            
        except Exception as e:
            messagebox.showerror("Error", f"Failed to plot layers: {str(e)}")
            self.status_label.config(text="Error plotting layers")

    def save_selected_layers(self):
        """Save the selected layers using save_dxf function"""
        if not self.vectors_data:
            messagebox.showwarning("Warning", "Please process the DXF file first")
            return
            
        if not self.selected_layers:
            messagebox.showwarning("Warning", "Please select at least one layer to save")
            return
            
        try:
            # Convert selected_layers set to list for save_dxf
            selected_layers_list = list(self.selected_layers)
            save_dxf(self.vectors_data, selected_layers_list)
            
            self.status_label.config(text=f"Saved {len(self.selected_layers)} layers to data/dxf/temp.pkl")
            messagebox.showinfo("Success", f"Successfully saved {len(self.selected_layers)} layers to data/dxf/temp.pkl")
            
        except Exception as e:
            messagebox.showerror("Error", f"Failed to save layers: {str(e)}")
            self.status_label.config(text="Error saving layers")
            
    def update_detailed_analysis(self):
        """Update the detailed analysis tab with segment-by-segment breakdown"""
        if not self.vectors_data:
            messagebox.showwarning("Warning", "Please process the DXF file first")
            return
            
        if len(self.selected_layers) != 1:
            messagebox.showwarning("Warning", "Please select exactly one layer for detailed analysis")
            return
            
        layer_name = list(self.selected_layers)[0]
        if layer_name not in self.vectors_data:
            messagebox.showwarning("Warning", f"Layer '{layer_name}' not found in processed data")
            return
            
        try:
            # Clear previous plot
            self.ax_analysis.clear()
            
            vectors = self.vectors_data[layer_name]
            
            # Generate distinct colors for each segment
            num_segments = len(vectors)
            colors = plt.cm.tab20(np.linspace(0, 1, num_segments))
            
            # Plot each segment with different color and number
            for i, vector in enumerate(vectors):
                if vector is not None and len(vector) > 0:
                    color = colors[i % len(colors)]
                    
                    if len(vector.shape) == 2:  # Array of points
                        if vector.shape[1] >= 2:  # At least 2D
                            x_coords = vector[:, 0]
                            y_coords = vector[:, 1]
                            self.ax_analysis.plot(x_coords, y_coords, color=color, linewidth=2, 
                                                label=f"Segment {i+1}")
                            
                            # Add segment number at midpoint
                            if len(x_coords) > 0:
                                mid_idx = len(x_coords) // 2
                                self.ax_analysis.annotate(f"{i+1}", 
                                                        (x_coords[mid_idx], y_coords[mid_idx]),
                                                        color=color, fontweight='bold', fontsize=10)
                    elif len(vector.shape) == 1:  # Single point
                        if len(vector) >= 2:
                            self.ax_analysis.plot(vector[0], vector[1], 'o', color=color, 
                                                markersize=5, label=f"Segment {i+1}")
                            self.ax_analysis.annotate(f"{i+1}", 
                                                    (vector[0], vector[1]),
                                                    color=color, fontweight='bold', fontsize=10)
            
            # Set plot properties
            self.ax_analysis.set_title(f"Detailed Analysis: {layer_name} ({num_segments} segments)")
            self.ax_analysis.set_xlabel("X")
            self.ax_analysis.set_ylabel("Y")
            self.ax_analysis.grid(True, alpha=0.3)
            self.ax_analysis.set_aspect('equal')
            
            # Add legend
            self.ax_analysis.legend(bbox_to_anchor=(1.05, 1), loc='upper left')
            
            # Update canvas
            self.canvas_analysis.draw()
            
        except Exception as e:
            messagebox.showerror("Error", f"Failed to update detailed analysis: {str(e)}")
            self.status_label.config(text="Error updating detailed analysis")

def main():
    root = tk.Tk()
    app = DXFViewerGUI(root)
    root.mainloop()

if __name__ == "__main__":
    main()
