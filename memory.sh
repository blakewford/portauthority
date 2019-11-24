FILE=/proc/$1/status
while test -f "$FILE"; do
    sleep 5
    date > memory.txt
    grep VmPeak /proc/$1/status >> memory.txt
done
