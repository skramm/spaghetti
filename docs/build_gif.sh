#!/bin/bash
convert -loop 0 \
\( sample_3_0.svg -set delay 30 \) \
\( sample_3_1.svg -set delay 40 \) \
\( sample_3_2.svg -set delay 50 \) \
\( sample_3_3.svg -set delay 60 \) \
\( sample_3_4.svg -set delay 70 \) \
out.gif

