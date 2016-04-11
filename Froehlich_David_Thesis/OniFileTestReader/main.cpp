#include "OpenNI.h"

#include <stdio.h>
#include <string>
#include <iostream>


using namespace std;
using namespace openni; 

int main() {
	Device ni_dev; 
	Status stat = OpenNI::initialize();
	if (stat == Status::STATUS_OK) {
		cout << "init succeeded" << endl;
	}
	else {
		cout << "nope..." << endl;
	}
	getchar();
	
	Status open_status = ni_dev.open("C:\\Users\\Groundstation\\Desktop\\recording_2016-03-02_10-26-27.oni");
	if (open_status == Status::STATUS_OK) {
		cout << "WHOOOO" << endl;
		getchar();
	}
	return 0;
}