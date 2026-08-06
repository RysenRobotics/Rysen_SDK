# 2D tactile image with tangential force (mirrors rysen::TactileImage)
uint32 width          # image width (pixels)
uint32 height         # image height (pixels)
uint32[] gray_image   # normal force image, unit Pa
TangentialForce tangential_forces  # tangential force for this patch
