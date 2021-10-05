echo -en "\033]0;Concurrent Clients\a"

gnome-terminal -e "bash -c 'gcc server.c -o serv.out -ldl -lpthread; ./serv.out sockfile 20 130 10'" --title "Server Terminal"
sleep 1

gcc client.c -o client.out -ldl -lpthread

num_clients=100 # Control Number of Clients to give

pids=($(seq 1 1 $num_clients))

for i in "${pids[@]}"; do
    "TestCase: $i"
    ./client.out sockfile "/lib/x86_64-linux-gnu/libwrap.so.0" sleep 2 &
    pids[$i]=$!
done

# wait for all pids
i=0
for pid in ${pids[*]}; do
    wait $pid
    echo "DONE CLIENT NUMBER: #$i"
    ((i=i+1))
done

echo $pid_server
read -n 1 -s -r -p "Press any key to continue"
./client.out sockfile "kill"
