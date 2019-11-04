#!/bin/bash
$@ &
PID=$!
kill -STOP $PID
echo $PID
wait $PID
