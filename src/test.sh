n=4
for (( i=0; i<$n; i++ ))
do
  echo "Test process #$i starts."
  min=$(( $n - $i ))
  echo "Running for $min min."
  ./client $min &
  sleep 1m
done
