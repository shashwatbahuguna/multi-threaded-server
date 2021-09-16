gcc client.c -o client.out -ldl -lpthread

num_clients=100 # Control Number of Clients to give

# clients=($(seq 1 1 $num_clients))
pids=($(seq 1 1 $num_clients))

for i in "${pids[@]}"; do
    ./client.out sockfile "/lib/x86_64-linux-gnu/libwrap.so.0" sleep 2 "TestCase: $i" &
    pids[$i]=$!
done

# wait for all pids
i=0
for pid in ${pids[*]}; do
    # echo "WAIT       $i"
    wait $pid
    echo "DONE CLIENT NUMBER: #$i"
    ((i=i+1))
done

read -n 1 -s -r -p "Press any key to continue"