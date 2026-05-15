# ==========================================
# 1. 基础环境加载
# ==========================================
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
if [ -f "$SCRIPT_DIR/install/setup.bash" ]; then
    source "$SCRIPT_DIR/install/setup.bash"
else
    echo "❌ 找不到 $SCRIPT_DIR/install/setup.bash！请确保已编译。"
    exit 1
fi

# ==========================================
# 2. 🧹 环境清理：释放端口与旧进程
# ==========================================
killall -9 foxglove_bridge 2>/dev/null
killall -9 rysen_apexhand_node_exe 2>/dev/null
killall -9 rysen_apexhand_choreography_node_exe 2>/dev/null
sleep 1

# ==========================================
# 3. 🛑 安全机制：捕获 Ctrl+C，一键退出
# ==========================================
cleanup() {
    echo -e "\n🛑 接收到停止信号！正在安全关闭所有节点..."
    killall -9 foxglove_bridge 2>/dev/null
    killall -9 rysen_apexhand_node_exe 2>/dev/null
    killall -9 rysen_apexhand_choreography_node_exe 2>/dev/null
    exit
}
trap cleanup SIGINT SIGTERM

# ==========================================
# 4. 🌐 打印前端连接信息
# ==========================================
echo "==================================================="
echo "🎯 请注意："
echo "请在 Foxglove 或前端 UI 中连接以下 WebSocket 地址："
ALL_IPS=$(hostname -I)
for IP in $ALL_IPS; do
  # 过滤掉 docker 的虚拟网段和本地回环，找出真实的局域网 IP
  if [[ $IP != 172.17.* && $IP != 172.18.* && $IP != 127.* ]]; then
      echo "    👉 ws://${IP}:8765"
  fi
done
echo "==================================================="
echo "⏳ 正在拉起 [驱动层] + [编排中枢] + [Foxglove]..."
echo "💡 提示：保持此终端运行。按 Ctrl+C 安全退出。"

# ==========================================
# 5. 🚀 终极启动：一条命令拉起整个宇宙
# ==========================================
ros2 launch rysen_apexhand rysen_apexhand.launch.py \
  launch_foxglove_bridge:=true \
  launch_choreography:=true