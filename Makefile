Targa = plot_graph

all:
	cat plot_graph.head.py > $(Targa).py
.for i in _*.py
	cat $i >> $(Targa).py
.endfor
	cat plot_graph.main.py >> $(Targa).py
	chmod 755 $(Targa).py

start:
	./$(Targa).py pwr_stat_12_02_2023.log

install:


clean:
	rm -f $(Targa).py
