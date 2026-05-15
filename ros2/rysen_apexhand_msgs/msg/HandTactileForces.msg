#! Hand-level tactile image + tangential forces from HandSensorImage
#! Mirrors rysen::HandSensorImage (only tactile part)

builtin_interfaces/Time stamp

CommonFingerTactile index    # 食指 (prox/mid/dist_pad + 切向力，见 NAMING_CONVENTION.md)
CommonFingerTactile middle   # 中指
CommonFingerTactile ring     # 无名指
CommonFingerTactile little   # 小拇指
ThumbFingerTactile thumb     # 大拇指

TactileImage palm_center     # 手掌中心 palm_center
