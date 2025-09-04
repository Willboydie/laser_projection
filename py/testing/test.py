import DXF.dxf_processing as dxf_processing
import geometry.shapes as shapes
import geometry.simplify as geom
import save
import numpy as np
import spherical.spherical as spherical


if __name__ == "__main__":
    dxf_processing.save_dxf('../../data/dxf/LowerBroich.dxf', 0.01, ['INTERNAL_WALLS'])
    