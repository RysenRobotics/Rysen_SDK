# 状态常量枚举
uint8 STATE_IDLE = 0           # 空闲待命状态 (初始状态或播放结束)
uint8 STATE_PLAYING = 1        # 正在正常播放
uint8 STATE_PAUSED = 2         # 已暂停
uint8 STATE_HOLDING = 3        # 正在关键帧停留
uint8 STATE_TRANSITIONING = 4  # 正在进行循环的首尾过渡
uint8 STATE_RESETTING = 5      # 正在执行柔性复位归零动作
uint8 STATE_HOMING = 6         # 正在回零（可能是回到第0帧/回到全0）
uint8 STATE_RESUMING = 7       # 从暂停被掰动后的柔性拉回状态
uint8 STATE_PAUSING = 8        # 动作结束时的目标跟踪状态

string ip                      # 表示这是哪只手的状态
uint8 state                    # 当前的内部状态机所处状态
float64 current_time           # 当前播放进度 (时间轴上的秒数)
float64 max_time               # 当前动作的总时长 (秒)