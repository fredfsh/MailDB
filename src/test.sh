n=$@
for (( i=1; i<=$n; i++ ))
do
  echo "Test process #$i starts."
  sec=$(( ($n - $i) * 30 + 60 ))
  echo "Running for $sec seconds."
  ./client $sec &
  sleep 30s
done
