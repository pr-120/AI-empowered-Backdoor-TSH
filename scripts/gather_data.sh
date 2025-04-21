#!/usr/bin/bash

# activate anaconda environment
source ~/anaconda3/bin/activate
conda activate py310

# Start the server in the background and save the process ID 
echo -e "\n ** STARTING SERVER **\n\n"
python ~/BA/server/roar_server/server.py -c &
SERVER_PID=$!
sleep 5

# kill server when script finishes
trap "echo 'Stopping server...'; kill $SERVER_PID" EXIT

# which configurations to run, key describes number of config
configurations=("normal" 0 1 2 3 4 5)

# select configs to gather data for
for config in "${configurations[@]}"; do
	
	# set config on client device
	expect -f ~/BA/scripts/set_config_on_client_device.exp $config
	# save config on server
	echo "{\"current_configuration\": \"$config\"}" > ~/BA/config/current_config.json

	# collect more data of normal workings
	if [ "$config" = "normal" ]; then
		number_of_fingerprints_to_be_made=100
			
		# starts fingerprinting process on client device
		expect -f ~/BA/scripts/start_fingerprinting.exp $number_of_fingerprints_to_be_made 
		
		# terminates when fp is finished (not needed in configs with malicious behavior as this functionality is directly in start_malicious_process.exp)
		expect -f ~/BA/scripts/check_fp_finished.exp
		
	else
		number_of_fingerprints_to_be_made=5
			
		# starts fingerprinting process on client device
		expect -f ~/BA/scripts/start_fingerprinting.exp $number_of_fingerprints_to_be_made 

		# create folder for stolen files to be stored in
		stolen_files_folder_location="/home/patrik/BA/stolen_files"
		mkdir -p $stolen_files_folder_location
	
		# start malicious process
		expect -f ~/BA/scripts/start_malicious_process.exp $stolen_files_folder_location 	

	fi	
	
done
