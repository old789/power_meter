def normalize_data():
	global data_timestamp, data_value, data_timestamp_norm, data_value_norm
	prev_timestamp=0
	i=0

	for i in range(0, len(data_timestamp)):
		if (i == 0) or ((data_timestamp[i]-prev_timestamp) == 1):
			data_timestamp_norm.append(data_timestamp[i])
			data_value_norm.append(data_value[i])
		elif (data_timestamp[i]-prev_timestamp) < 45:
			fill_data(prev_timestamp,data_timestamp[i],data_value[i])
		else:
			fill_data(prev_timestamp,data_timestamp[i],0)
		prev_timestamp=data_timestamp[i]

