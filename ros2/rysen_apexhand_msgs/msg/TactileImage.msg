# 2D tactile image with tangential force (mirrors rysen::TactileImage)
uint32 width          # image width (pixels)
uint32 height         # image height (pixels)
uint16[] gray_image   # grayscale image data (row-major, size = width*height)
TangentialForce tangential_forces  # tangential force for this patch
