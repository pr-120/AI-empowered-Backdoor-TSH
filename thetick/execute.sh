#!/bin/bash

# activate base anaconda environment
cd ~/anaconda3/bin
source activate

# activate specific conda environment
conda activate py27

# start the tick console interactively
expect ~/BA/thetick/expect_test_script.exp

