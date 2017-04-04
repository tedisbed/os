# os

Teddy Brombach and Tristan Mitchell

In order to get our code working you need to pass in a config file that is
correctly formatted. We have one names "param.txt". when you pass this in it
will start pulling the sites that are in the sites file that you pass into the
program via your config file. After doing this it will start looking for
instances of specified words that are passed to the function by the config file.
The results that the program finds are written out to csv files. the names of
which increase every time the program curls the website. Some interesting things
we chose to do (via the TA's suggestions) was to limit the number of threads
that can by made. If there are more parse threads than parse words, we dont make
the extra threads. This is because their whole life they would be asleep. We
chose to do the same things with curl (fectch) threads, for the same reason. we
also chose to use usleep instead of sig alaram, again because a TA said that it
would be ok to do our sig alarm this way. 

config file possible parameters

Period fetch: the time between fetches
num fetch: number of fetch threads
num parse: number of parse threads
search file: the file that has all the words you want to search
site file: the file that has all the sites you want to search
