set terminal png size 1400,200 crop
set output 'out.png'
#set xlabel " "

#set ytics ("Off" 0, "On" 1)
#set y2tics ("Off" 0, "On" 1)
set yrange [-0.1:1.1]
set y2range [-0.1:1.1]

plot "< awk '{print $1, $2==\"r\"? 1 : 0; }' out" using 1:2 notitle with steps
