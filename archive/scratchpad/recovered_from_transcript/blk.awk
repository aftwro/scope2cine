# block-edge energy: mean |dx| at 8-pixel borders vs elsewhere, over od -tu1 -w1280 rows
{ for (i = 2; i <= NF; i++) { d = $i - $(i-1); if (d < 0) d = -d; if ((i-1) % 8 == 0) { e += d; ne++ } else { o += d; no++ } } }
END { printf "DEV block-edge %.2f other %.2f ratio %.2f\n", e/ne, o/no, (e/ne)/(o/no) }
