reset
set xlabel 'thread no.'
set ylabel 'time (us)'
set title 'kecho UNBOUND performance'
set term png enhanced font 'Verdana,10'
set output 'kecho UNBOUND bench.png'
set key left

plot [0:][0:] \
'bench.txt' using 1:2 with points title 'Avg. time elapsed'
