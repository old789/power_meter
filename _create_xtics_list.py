def create_xtics_list():
	global data_timestamp_norm, xticks_lst
	step_xtick=3600
	prev_timestamp=data_timestamp_norm[0]-step_xtick
	tick=0

	for tick in range(0,len(data_timestamp_norm)):
		if (data_timestamp_norm[tick]-prev_timestamp) >= step_xtick:
			prev_timestamp=data_timestamp_norm[tick]
			xticks_lst.append(tick)

