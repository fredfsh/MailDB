n=$@
for (( i=1; i<=$n; i++ ))
do
  echo "Test process #$i starts."
  sec=$(( ($n - $i) * 10 + 30 ))
  echo "Running for $sec seconds."
  ./client $sec &
  sleep 10s
done
