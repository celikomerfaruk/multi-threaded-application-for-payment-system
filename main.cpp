#include <iostream>
#include <fstream>
#include <unistd.h>
#include <pthread.h>
#include <sstream>

using namespace std;

struct customer {
    int id;
    int sleepTime;
    int ticketVendingMachineInstance;
    int prepaymentCompany;
    int amount;
};
// Global variables.
ofstream outputf;

int totalAmount[5] = {0, 0, 0, 0, 0};
int numberOfCustomers;
int machine_volume[11]; //holds remaining prepayment actions for each vending machine

pthread_mutex_t vending_machine_lock[10];  
pthread_mutex_t customer_lock[10];

customer cus_at_vending[10]; //holds customer info assigned to each vending machine

pthread_mutex_t total_amount_lock[5];
pthread_mutex_t output_write_lock;

void *threadVendingMachine(void *param)  //vending machine thread runner
{
    int vending_id = *((int *) param);
    while(machine_volume[vending_id] > 0) {  //while there are still prepayment actions to be done
    
        if(cus_at_vending[vending_id].id == 0){
            continue;
        }
   
        pthread_mutex_lock(&vending_machine_lock[vending_id]);
        customer cus = cus_at_vending[vending_id];        //get customer info
        machine_volume[vending_id]--;         
        pthread_mutex_unlock(&customer_lock[vending_id]);   

        pthread_mutex_lock(&total_amount_lock[cus.prepaymentCompany]);
        totalAmount[cus.prepaymentCompany] += cus.amount;       //update total amount
        pthread_mutex_unlock(&total_amount_lock[cus.prepaymentCompany]);

        string prepaymentCompany;
        if(cus.prepaymentCompany == 0) {
            prepaymentCompany = "Kevin";
        } else if(cus.prepaymentCompany == 1) {
            prepaymentCompany = "Bob";
        } else if(cus.prepaymentCompany == 2) {
            prepaymentCompany = "Stuart";
        } else if(cus.prepaymentCompany == 3) {
            prepaymentCompany = "Otto";
        } else if(cus.prepaymentCompany == 4) {
            prepaymentCompany = "Dave";
        }

        pthread_mutex_lock(&output_write_lock);
        outputf<<"Customer"<<cus.id<<","<<cus.amount<<"TL,"<<prepaymentCompany<<endl;   //write to output file
        pthread_mutex_unlock(&output_write_lock);
        
        if(machine_volume[vending_id] == 0){  //if there are no more prepayment actions to be done for this vending machine free the lock
            pthread_mutex_unlock(&vending_machine_lock[vending_id]);
        }
        
    }
    pthread_exit(0);
}

void *threadCustomer(void *param)  //customer thread runner
{
    customer cus = *((struct customer *) param);
    usleep(cus.sleepTime * 1000);   //sleep for customer's sleep time

    pthread_mutex_lock(&customer_lock[cus.ticketVendingMachineInstance]);
    cus_at_vending[cus.ticketVendingMachineInstance] = cus;     //assign customer to vending machine
    pthread_mutex_unlock(&vending_machine_lock[cus.ticketVendingMachineInstance]);   

    pthread_exit(0);
}

int main(int argc, char *argv[])  
{   
    if(argc != 2) {
        return 1;
    }
    ifstream inputf(argv[1]);
    string outputf_str= argv[1];
    outputf_str.erase(outputf_str.end()-4, outputf_str.end());
    outputf_str += "_log.txt";
    outputf.open(outputf_str);    
    
    string line;
    getline(inputf, line);
    numberOfCustomers = stoi(line);

    customer customers[numberOfCustomers];
    for(int i = 0; i < numberOfCustomers; i++) {    //read input file and assign customer info
        customers[i].id = i+1;
        string line;
        getline(inputf, line);
        stringstream ss(line);
        string token;
        int j = 0;
        while(getline(ss, token, ',')) {
            if(j == 0) {
                customers[i].sleepTime = stoi(token);
            } else if(j == 1) {
                customers[i].ticketVendingMachineInstance = stoi(token);
                machine_volume[customers[i].ticketVendingMachineInstance] += 1;     //update remaining prepayment actions for each vending machine
            } else if(j == 2) {
                if(token == "Kevin") {
                    customers[i].prepaymentCompany = 0;
                } else if(token == "Bob") {
                    customers[i].prepaymentCompany = 1;
                } else if(token == "Stuart") {
                    customers[i].prepaymentCompany = 2;
                } else if(token == "Otto") {
                    customers[i].prepaymentCompany = 3;
                } else if(token == "Dave") {
                    customers[i].prepaymentCompany = 4;
                }
            } else if(j == 3) {
                customers[i].amount = stoi(token);
            }
            j++;
        }        
    }

    output_write_lock = PTHREAD_MUTEX_INITIALIZER;  //initialize locks
    for(int i = 0; i < 10; i++) {  
        vending_machine_lock[i] = PTHREAD_MUTEX_INITIALIZER;
        customer_lock[i] = PTHREAD_MUTEX_INITIALIZER;
        pthread_mutex_lock(&vending_machine_lock[i]);   //firstly lock vending machines and don't let them run until customers are assigned
    }

    pthread_attr_t attr;
    pthread_attr_init(&attr);

    pthread_t vending_machine_threads[10];
    pthread_t customer_threads[numberOfCustomers];

    int vending_ids[10];

    for(int i = 0; i < 10; i++) {
        vending_ids[i] = i+1;
        pthread_create(&vending_machine_threads[i], &attr, threadVendingMachine, &vending_ids[i]);  //create vending machine threads
    }

    for(int i = 0; i < numberOfCustomers; i++) {
        pthread_create(&customer_threads[i], &attr, threadCustomer, &customers[i]); //create customer threads
    }

    for(int i = 0 ; i < numberOfCustomers ; i++){
        pthread_join(customer_threads[i], NULL);    //wait for all customers to finish
    }

    for(int i = 0 ; i < 10 ; i++){
        pthread_join(vending_machine_threads[i], NULL); //wait for all vending machines to finish
    }

    pthread_mutex_lock(&output_write_lock);  //write total amount of each company to output file
    outputf<<"All prepayments are completed."<<endl;
	outputf<<"Kevin: "<<totalAmount[0]<<"TL"<<endl;
	outputf<<"Bob: "<<totalAmount[1]<<"TL"<<endl;
	outputf<<"Stuart: "<<totalAmount[2]<<"TL"<<endl;
	outputf<<"Otto: "<<totalAmount[3]<<"TL"<<endl;
	outputf<<"Dave: "<<totalAmount[4]<<"TL"<<endl;
    pthread_mutex_unlock(&output_write_lock);

    inputf.close();
    outputf.close();

    for (int i = 0; i < 10; i++) {  //destroy locks
        pthread_mutex_destroy(&vending_machine_lock[i]);    
        pthread_mutex_destroy(&customer_lock[i]);
        if (i < 5) {
            pthread_mutex_destroy(&total_amount_lock[i]);
        }
    }
    pthread_mutex_destroy(&output_write_lock);
    
    return 0;
}
