# usage
if [ $# -ne 2 ]; then
  echo "Usage: ./test <type> <process_num>";
  echo "<type> is one of \"read\", \"write\", \"mixed\"";
  exit;
fi

# clears previous result files
rm *0

# number of test processes
n=$2

for (( i=1; i<=$n; i++ ))
do
  # test time in seconds for process i
  sec=$(( ($n - $i) * 60 + 60 ))

  echo "Test process #$i starts."
  echo "Running for $sec seconds."
  ./client $1 $sec &

  # another test process starts some time later
  sleep 60s

done
