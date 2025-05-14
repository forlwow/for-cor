+ 当前连接数
netstat -ant|awk '/^tcp/ {++S[$NF]} END {for(a in S) print (a,S[a])}'

lsof -p <your_program_pid> | wc -l

+ ip与端口范围
cat /proc/sys/net/ipv4/ip_local_port_range

+ 打开描述符
cat /proc/$$/limits | grep "open files"

+ 统计行数
find . \( -path "./ThirdModule" \) -prune -o \( -name "*.cpp" -o -name "*.h" \) -
print | xargs cat | wc -l