#!/usr/bin/env python3
"""
Simple runner script for the DXF Layer Viewer GUI
"""

import sys
import os

# Add the current directory to the Python path to import dxf_processing
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

try:
    from dxf_gui import main
    print("Starting DXF Layer Viewer GUI...")
    main()
except ImportError as e:
    print(f"Import error: {e}")
    print("Please ensure all required packages are installed:")
    print("pip install -r requirements.txt")
except Exception as e:
    print(f"Error starting GUI: {e}")
    print("Please check that all dependencies are properly installed.")
