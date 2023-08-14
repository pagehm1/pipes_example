/****************************************
*Name: Hunter Page, pagehm1@etsu.edu
*Class: CSCI 4727-001
*Date Last Modified: 10/25/21
****DO FINAL READOVER AND TEST IN EINSTEIN
****************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "product_record.h"
#include <unistd.h>
#include <sys/wait.h>
#include "modifyFile.h"

//the first number declares the amount of pipes
//second number declares the input value and output value
//5 pipes for between each process and one for writing to parent
int fd[6][2];

int i; //for loop number; total that calculates a continual total of the orders
double runningTotal; //keeps track of the total cost of all entries

/*Calculates the tax amount of a product record*/
void stationZero(struct product_record* productInfo)
{
	productInfo->tax = productInfo->price * productInfo->number * 0.05;
	
}

/*Calculates the shipping and handling of the product*/
void stationOne(struct product_record* productInfo)
{
	productInfo->sANDh = (productInfo->price *productInfo->number * 0.01) + 10;
	productInfo->stations[1] = 1;
}

/*grabs the total price of the product record*/
void stationTwo(struct product_record* productInfo)
{
	productInfo->total = (productInfo->price * productInfo->number) + productInfo->tax + productInfo->sANDh;
	productInfo->stations[2] = 1;
}

/*updates the entire cost of all product records*/
void stationThree(struct product_record* productInfo)
{
	runningTotal = runningTotal + productInfo->total;
	printf("\n\nRunning total: %.2f\n\n", runningTotal); //print the current running total
	productInfo->stations[3] = 1;
}

/*prints the product record after all of the other stations*/
void stationFour(struct product_record* productInfo)
{
	productInfo->stations[4] = 1;
	printStruct(*productInfo);
}


/*
 *Creates five processes and six pipes to communicate that will read in the product records 
	and perform calculations to fill the record's missing info. Then it will 
	write the record to a new file. Each pipe is linked between each process, with the sixth
	process connecting back to the parent.
	
	argv[1] - input file
	argv[2] - output file
*/
int main(int argc, const char* argv[])
{
	int rows;    //number of rows (number of pipes)
	rows = sizeof(fd) / sizeof(fd[0]);
	runningTotal = 0;
	
	struct product_record productInfo; //struct that stores the info read in from the file
    FILE* inputFile = fopen(argv[1], "r"); //makes the input file from file passed in
	FILE* outputFile = fopen(argv[2], "w+"); //makes the ouput file from the name passed in
	
	
	//initialize the pipes
	for(i=0; i < rows; i++)
	{
		int pipeReturn = pipe(fd[i]); //create pipe for the array
		if(pipeReturn < 0) //check for errors
		{
			printf("There was an error creating a pipe, exiting.");
			return 0;
		}
		
		printf("Pipe %d was created.\n", i);
	}
	
	//create an array of the child id's
	int childProcesses[sizeof(productInfo.stations)];
	
	
	//initialize all of the child processes by checking for the
	//parent to fork
	for(i = 0; i < 5; i++)
	{
		//if it is the child process then do not fork
		if((childProcesses[i] = fork()) == 0)
		{
			break;
		}
		if(childProcesses[i] == -1) //check for an error
		{
			printf("There was an error, exiting...\n\n");
		}
		
		printf("Process %d was created.\n", childProcesses[i]);
	}
	
		
	if(childProcesses[0] == 0)  //zero station - find tax
	{
		struct product_record statZero;
		int numRecords;
		
		//run until parent sends ending signal
		while(1)
		{
			//wait(NULL); //wait for parent to give an entry from the file
			read(fd[0][0], &statZero, sizeof(statZero));
			
			//leave if ending id is read
			if(statZero.idnumber == -1)
			{
				break;
			}
			
			if(statZero.idnumber >= 1000) //special product does not need its tax computed
			{
				printf("Special order, no tax computed.\n");
			}
			else //perform calculations
			{
				stationZero(&statZero);
				numRecords = numRecords + 1;
			}
			statZero.stations[0] = 1; //increment the station's number
			write(fd[1][1], &statZero, sizeof(statZero)); //write to the next pipe
			
		}
		
		printf("Process 0 has processed %d products.\n", numRecords);
		exit(0);
		
	}
	if(childProcesses[1] == 0) //first station - find shipping and handling
	{
		struct product_record statOne;
		int numRecords;
		
		//run until parent sends ending signal
		while(1)
		{
			//wait(NULL); //wait for zero child to give an entry from the file
			read(fd[1][0], &statOne, sizeof(statOne));
			
			if(statOne.idnumber == -1) //check for end signal
			{
				break;
			}
			
			stationOne(&statOne); //perform calculations
			write(fd[2][1], &statOne, sizeof(statOne));
			numRecords = numRecords + 1;
		}
		printf("Process 1 has processed %d products.\n", numRecords);
		exit(0);
	}
	if(childProcesses[2] == 0) //second station - find total for record
	{
		struct product_record statTwo;
		int numRecords;
		
		//run until parent sends ending signal
		while(1)
		{
			//wait(NULL); //wait for first child to give an entry from the file
			read(fd[2][0], &statTwo, sizeof(statTwo));
			
			if(statTwo.idnumber == -1) //check for end signal
			{
				break;
			}
			
			stationTwo(&statTwo); //perform calculations
			write(fd[3][1], &statTwo, sizeof(statTwo));
			numRecords = numRecords + 1;
			
		}
		printf("Process 2 has processed %d products.\n", numRecords);		
		exit(0);
	}
	if(childProcesses[3] == 0) //third station - add to running total
	{
		struct product_record statThree;
		int numRecords;
		
		//run until parent sends ending signal
		while(1)
		{
			//wait(NULL); //wait for first child to give an entry from the file
			read(fd[3][0], &statThree, sizeof(statThree));
			
			if(statThree.idnumber == -1) //check for end signal
			{
				break;
			}
			
			stationThree(&statThree); //perform calculations
			write(fd[4][1], &statThree, sizeof(statThree));
			numRecords = numRecords + 1;
		}
		printf("Process 3 has processed %d products.\n", numRecords);
		exit(0);
	}
	if(childProcesses[4] == 0) //fourth station - print updated record
	{
		struct product_record statFour;
		int numRecords;
		
		//run until parent sends ending signal
		while(1)
		{
			//wait(NULL); //wait for first child to give an entry from the file
			read(fd[4][0], &statFour, sizeof(statFour));
			
			if(statFour.idnumber == -1) //check for end signal
			{
				break;
			}
			
			stationFour(&statFour); //print the updated record
			write(fd[5][1], &statFour, sizeof(statFour));
			numRecords = numRecords + 1;
		}
		printf("Process 4 has processed %d products.\n", numRecords);
		exit(0);
	}
	else //parent code
	{
		struct product_record parentStruct;
		int numRecords;
		
		//enter each order from the input file into the output file one at a time
		while(!feof(inputFile))
		{
			readFile(inputFile, &parentStruct); //read the file to get one set of product information
		
			write(fd[0][1], &parentStruct, sizeof(parentStruct)); //write the struct to the pipe
					
			read(fd[5][0], &parentStruct, sizeof(parentStruct));
		
			writeFile(outputFile, parentStruct); //write the recorded product info into the new file
			
			numRecords++;
		}
		
		parentStruct.idnumber = -1;
		
		//write to all pipes that the last entry has been made
		for(i = 0; i < 5; i++)
		{
			write(fd[i][1], &parentStruct, sizeof(parentStruct));
		}
		
		printf("Parent process processed %d products.\n", numRecords);
		
		//close both files
		fclose(inputFile);
		fclose(outputFile);
		
		return 0;
	}
}