reset
set xlabel 'thread no.'
set ylabel 'time (us)'
set title 'kecho HIGHPRI|INTENSIVE performance'
set term png enhanced font 'Verdana,10'
set output 'kecho HIGHPRI|INTENSIVE bench.png'
set key left

plot [0:][0:] \
'bench.txt' using 1:2 with points title 'Avg. time elapsed'
