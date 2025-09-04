# EZDXF Documentation:
https://ezdxf.readthedocs.io/en/stable/index.html

# DXF Layer Viewer - Quick Start Guide

## 🚀 Get Started in 3 Steps

### 1. Install Dependencies
```bash
cd py/DXF
pip install -r requirements.txt
```

### 2. Test the Installation
```bash
python test_gui.py
```
You should see: "🎉 All tests passed! The GUI should work correctly."

### 3. Launch the GUI
```bash
python dxf_gui.py
```

## 📁 What You Get

- **File Browser**: Click "Browse DXF File" to select your DXF
- **Layer Explorer**: See all available layers with entity types
- **Interactive Plot**: Visualize selected layers with zoom/pan
- **Processing Options**: Choose which sections to analyze

## 🎯 First Steps

1. **Load a DXF file** (like the included `LowerBroich.dxf`)
2. **Click "Process DXF File"** to extract geometry
3. **Select layers** from the list (Ctrl+Click for multiple)
4. **Click "Plot Selected Layers"** to visualize

## 🔧 Troubleshooting

- **Import errors**: Run `pip install -r requirements.txt`
- **No plot**: Make sure to process the file first
- **Slow performance**: Reduce epsilon value or uncheck unused sections

## 📖 Full Documentation

See `README_GUI.md` for complete details and advanced features.

## 🎨 Sample Files

- `LowerBroich.dxf` - Complex architectural drawing (21 layers)
- Test with your own DXF files for different results

---
*Ready to explore your DXF files? Launch the GUI and start visualizing!*
