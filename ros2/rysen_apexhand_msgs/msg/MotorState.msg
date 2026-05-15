# Standard motor state message (similar to sensor_msgs/JointState)

# Header with timestamp
std_msgs/Header header

# Motor names (order matches MotorId / rysen_apexhand_node, e.g. thumb_cmc_abd_motor, ...)
string[] name

# Motor temperatures (in degrees Celsius)
float64[] temperature

# Motor currents (in Amperes)
float64[] current
