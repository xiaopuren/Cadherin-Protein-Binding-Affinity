//  (1)Add cluster diffusion (2) Add check point file
//  main.cpp
//  Cadherin2
//
//  Created by Zhaoqian Su on 1/14/19.
//  Copyright (c) 2019 @AE. All rights reserved.
//

//
//  main.cpp
//  Cadherin
//
//  Created by Zhaoqian Su on 10/18/18.
//  Copyright (c) 2018 @AE. All rights reserved.
//


#include <iostream>
#include <fstream>
#include <random>
#include <chrono>
#include <math.h>
#include <iomanip> //needed to use manipulators with parameters (precision, width)
#include <queue>    // storage index of bonded protein index
#include <set>      // storage complex number
#include <vector>
#include <algorithm>


using namespace std;

int simu_step = 1000000000;
double time_step = 10; //nano second
double distance_step = 10; //Angstrom
double distance_amp;
double cell_range_x = 4000; //Angstrom
double cell_range_y = 4000;
double cell_range_z = 120;

#define RB_A_tot_num 500
#define protein_A_tot_num 100
#define RB_A_num_per_protein 5
#define RB_A_res_num 5

#define protein_A_tot_num_matrix 101
#define RB_A_num_per_protein_matrix 6
#define RB_A_res_num_matrix 6

#define protein_tot_num 100
#define max_bond_num 200  // 2* protein_A_tot_num

#define protein_tot_num_matrix 101
#define max_bond_num_matrix 201  // 2* protein_A_tot_num +1

double pai = 3.1415926;
double RB_A_radius = 15;     // receptor
double RB_A_D = 1;           // parameter from mebrane protein of plosOne paper   // Unit: A^2/ns  = 10 mum^2/s
double RB_A_rot_D = 0.0174;   //

double mono_cis_Ass_Rate = 0;
double mono_cis_Diss_Rate = 0;

double cis_D = 0.5;      // two different cis interaction: (1) two single receptors (2)
double cis_rot_D = 0.005;
double cis_Ass_Rate = 0.09;
double cis_Diss_Rate = 0.0000001;

double bond_D = 0.5;
double bond_rot_D = 0.005;
double Ass_Rate = 0.09; //units: per nanosecond
double Diss_Rate = 0.0;

double bond_dist_cutoff = 7;
double bond_thetapd = 90;
double bond_thetapd_cutoff = 15;
double bond_thetaot = 180;
double bond_thetaot_cutoff = 15;
double cis_thetaot_cutoff = 30;
double cis_dist_cutoff = 12;

double bond_D_cal, bond_rot_D_cal;
double R_x [protein_tot_num_matrix][RB_A_num_per_protein_matrix][RB_A_res_num_matrix];  //matrix size must be constant, so use #define in the above
double R_y [protein_tot_num_matrix][RB_A_num_per_protein_matrix][RB_A_res_num_matrix];
double R_z [protein_tot_num_matrix][RB_A_num_per_protein_matrix][RB_A_res_num_matrix];
double R_x_0 [protein_tot_num_matrix][RB_A_num_per_protein_matrix][RB_A_res_num_matrix];
double R_y_0 [protein_tot_num_matrix][RB_A_num_per_protein_matrix][RB_A_res_num_matrix];
double R_z_0 [protein_tot_num_matrix][RB_A_num_per_protein_matrix][RB_A_res_num_matrix];
double R_x_new [protein_tot_num_matrix][RB_A_num_per_protein_matrix][RB_A_res_num_matrix];  //matrix in fortran and C++ are different starting from 0 or 1
double R_y_new [protein_tot_num_matrix][RB_A_num_per_protein_matrix][RB_A_res_num_matrix];
double R_z_new [protein_tot_num_matrix][RB_A_num_per_protein_matrix][RB_A_res_num_matrix];
double R_x_new0 [protein_tot_num_matrix][RB_A_num_per_protein_matrix][RB_A_res_num_matrix];
double R_y_new0 [protein_tot_num_matrix][RB_A_num_per_protein_matrix][RB_A_res_num_matrix];
double R_z_new0 [protein_tot_num_matrix][RB_A_num_per_protein_matrix][RB_A_res_num_matrix];

int protein_status[protein_tot_num_matrix][RB_A_num_per_protein_matrix];
int protein_status_new[protein_tot_num_matrix][RB_A_num_per_protein_matrix];
int res_nei[protein_tot_num_matrix][7];
int res_nei_new[protein_tot_num_matrix][7];
int seed[protein_tot_num_matrix];
int seed_new[protein_tot_num_matrix];
int visited[protein_tot_num_matrix];
int moved[protein_tot_num_matrix];

int results[protein_tot_num_matrix][protein_tot_num_matrix];  // use BFS to find out all RB index of each complex in the whole system.
int results_prev[protein_tot_num_matrix][protein_tot_num_matrix]; // use results_prev to storage the results of previous step
int results_new[protein_tot_num_matrix][protein_tot_num_matrix]; // use reults_new to find out newly attached proteins on seed
int results_temp[protein_tot_num_matrix][protein_tot_num_matrix]; // use results_prev to storage the results of previous step
int results_temp_size, results_prev_size;
int temp_int;

int complex_index; //complex_index corresponds to row number of results[][]
int complex_sub_index; // subunits index of a complex, from 1 to protein_tot_num
int protein_num_in_Max_Complex;

int bond_num_AB,bond_num_AC,bond_num_AD,bond_num_BC,bond_num_CD,bond_num_AB_new,bond_num_AC_new,bond_num_AD_new,bond_num_BC_new,bond_num_CD_new; //number of bond formed in the simulation
int bond_num_cis, bond_num_cis_new, bond_num_mono_cis, bond_num_mono_cis_new, complex_num; //number of bond formed in the simulation

double rand2(); //declare random number generator
double gettheta (double point_x[3], double point_y[3], double point_z[3], double test_theta); //declear the function for angle calculation
int i,j,k,n_t, m, n, b, it, jt, kt;
double temp_i,temp_j,temp_k;
double theta,phi,psai,phai;
double dist, dist1, dist2, dist3, dist4;
double cm1_a_x;
double cm1_a_y, cm0_a_y;
double cm1_a_z;
double t[3][3];
double PB_x, PB_y, PB_z;
double initial_simu_time, current_simu_time;
int mc_time_step;
int iteration_mole_step;
int selecting_mole_index; //select one molecule
int RB_A_index, RB_B_index, protein_A_index, protein_B_index, protein_C_index, protein_D_index, res_B_index1, res_B_index2,res_B_index3,protein_A_index1, protein_A_index2, protein_A_index3, q_index;
double Prob_diff;
int collision_flag;
double point_x[3], point_y[3], point_z[3];
double theta_pd, theta_ot;
double theta_pd2, theta_ot2;
double Prob_Ass, Prob_Diss;
double angle, angle1, angle2;
int selected_bond, selected_pro_A, selected_pro_A2, selected_pro_B, selected_pro_C, selected_pro_D, selected_res_B;
double prob;
double temp, angle_x1, angle_x2, angle_y1, angle_y2, dot, det;
int part1,part2,part3,part4,part5,part6,part7,part8,part9;
int complex_size, tot_cluster_num, tot_proteins_in_cluster;    // cluster_size = total proteins in clusters / total cluster_num
double cluster_size;
int protein_A_num_inComplex, protein_B_num_inComplex, protein_C_num_inComplex, protein_D_num_inComplex;
queue<int> q;

#define EPSILON    (1.0E-8)
bool AreSame(double a, double b);

std::ofstream parameter ("parameter.log");


int main (){
    
    //////////////write the input parameter to log file
    std::ofstream parameter ("parameter.log", std::ofstream::app);
    parameter <<setw(25)<< "box size: x y z" <<setw(15)<< cell_range_x << setw(7)<< cell_range_y << setw(7) << cell_range_z<<'\n'<<'\n';
    parameter <<setw(25)<< "protein_A_tot_num" <<setw(15)<< protein_A_tot_num <<'\n';
    parameter <<setw(25)<< "RB_A_tot_num" <<setw(15)<< RB_A_tot_num <<'\n';
    
    
    parameter <<setw(25)<< "RB_A_D" <<setw(15)<< RB_A_D <<'\n';
    parameter <<setw(25)<< "RB_A_rot_D" <<setw(15)<< RB_A_rot_D <<'\n';
    
    parameter <<setw(25)<< "R-L interaction:" <<'\n';
    parameter <<setw(25)<< "bond_D" <<setw(15)<< bond_D<<'\n';
    parameter <<setw(25)<< "bond_rot_D" <<setw(15)<< bond_rot_D<<'\n';
    parameter <<setw(25)<< "Ass_Rate" <<setw(15)<< Ass_Rate<<'\n';
    parameter <<setw(25)<< "Diss_Rate" <<setw(15)<< Diss_Rate<<'\n'<<'\n';
    
    parameter <<setw(25)<< "Cis interaction:" <<'\n';
    parameter <<setw(25)<< "cis_D" <<setw(15)<< cis_D<<'\n';
    parameter <<setw(25)<< "cis_rot_D" <<setw(15)<< cis_rot_D<<'\n';
    parameter <<setw(25)<< "mono_cis_Ass_Rate" <<setw(15)<< mono_cis_Ass_Rate<<'\n';
    parameter <<setw(25)<< "mono_cis_Diss_Rate" <<setw(15)<< mono_cis_Diss_Rate<<'\n'<<'\n';
    parameter <<setw(25)<< "cis_Ass_Rate" <<setw(15)<< cis_Ass_Rate<<'\n';
    parameter <<setw(25)<< "cis_Diss_Rate" <<setw(15)<< cis_Diss_Rate<<'\n'<<'\n';
    
    parameter.close();
    
    /////////////// initialize bond information
    for (i = 1; i <= protein_A_tot_num; i++){
        res_nei[i][1] = 0;
        res_nei[i][2] = 0;
        res_nei[i][3] = 0;
    }
    
    
    
    std::ifstream check_point ("position.cpt");
        
        if (check_point.is_open()){
            cout << "CPT file is exist\n";
            
            for (i = 1; i <= protein_A_tot_num; i++){
                for (j = 1; j <= RB_A_num_per_protein; j++){
                    for (k = 1; k <= RB_A_res_num; k++){
                        check_point >> R_x[i][j][k];
                        check_point >> R_y[i][j][k];
                        check_point >> R_z[i][j][k];
                        
                    }
                }
                check_point >> res_nei[i][1];
                check_point >> res_nei[i][2];
                check_point >> res_nei[i][3];
                
            }
            
            check_point >> bond_num_mono_cis;
            check_point >> initial_simu_time;
            initial_simu_time = initial_simu_time + 1;    // avoid output same cpt frame twice! previous frame = current frame
            
            
        } /////// end if cpt file exist
        //////  else randomly generate inital configuration
        
        else{
            cout << "CPT file not exist\n";
            std::ofstream ofs ("test.gro");  // open a new test.gro file, delete the previous
            std::ofstream bond ("bond.dat");  // open a new test.gro file, delete the previous
            std::ofstream cluster ("cluster.log");
            std::ofstream check_point ("position.cpt");

    //randomly insert RB_A,  A is membrane molecule in x-y plane
    for (i=1; i <= protein_A_tot_num; i++){
        
    lable1:
        temp_i=rand2()*cell_range_x - cell_range_x/2;
        temp_j=rand2()*cell_range_y - cell_range_y/2;
        temp_k= 0;
        
        //avoid overlap with RB_A
        for (j=1; j <= i-1; j++){
            dist=sqrt( (temp_i-R_x[j][1][1])*(temp_i-R_x[j][1][1])
                      +(temp_j-R_y[j][1][1])*(temp_j-R_y[j][1][1])
                      );
            if (dist <= RB_A_radius*2) {
                goto lable1;
            }
        }
        
        R_x[i][1][1] = temp_i;
        R_y[i][1][1] = temp_j;
        R_z[i][1][1] = temp_k + RB_A_radius; // change radius
        
        R_x_0[i][1][1] = temp_i;
        R_y_0[i][1][1] = temp_j;
        R_z_0[i][1][1] = temp_k + RB_A_radius;
        
        R_x_0[i][2][1] = temp_i;
        R_y_0[i][2][1] = temp_j;
        R_z_0[i][2][1] = temp_k + 3*RB_A_radius;
        
        R_x_0[i][3][1] = temp_i;
        R_y_0[i][3][1] = temp_j;
        R_z_0[i][3][1] = temp_k + 5*RB_A_radius;
        
        R_x_0[i][4][1] = temp_i;
        R_y_0[i][4][1] = temp_j;
        R_z_0[i][4][1] = temp_k + 7*RB_A_radius;
        
        R_x_0[i][5][1] = temp_i;
        R_y_0[i][5][1] = temp_j;
        R_z_0[i][5][1] = temp_k + 9*RB_A_radius;
        
        for (j = 1; j <= RB_A_num_per_protein; j++){
            
            R_x_0[i][j][2] = R_x_0[i][j][1] + RB_A_radius;
            R_y_0[i][j][2] = R_y_0[i][j][1];
            R_z_0[i][j][2] = R_z_0[i][j][1];
            
            R_x_0[i][j][3] = R_x_0[i][j][1] - RB_A_radius;
            R_y_0[i][j][3] = R_y_0[i][j][1];
            R_z_0[i][j][3] = R_z_0[i][j][1];
            
            R_x_0[i][j][4] = R_x_0[i][j][1];
            R_y_0[i][j][4] = R_y_0[i][j][1];
            R_z_0[i][j][4] = R_z_0[i][j][1] + RB_A_radius;
            
            R_x_0[i][j][5] = R_x_0[i][j][1];
            R_y_0[i][j][5] = R_y_0[i][j][1] - RB_A_radius;
            R_z_0[i][j][5] = R_z_0[i][j][1];
        }
        
        res_nei[i][1] = 0;  // index of protein_B binding with protein_A
        res_nei[i][2] = 0;  // which rb of protein_B binding with protein_A[i]
        res_nei[i][3] = 0;   // cis interaction, index of protein_A
        seed[i] = 0;
        
        
        
        
        //rotation...
        theta = 0;
        phi = 0;
        psai = (2*rand2() - 1)*pai;
        
        t[0][0] = cos(psai)*cos(phi) - cos(theta)*sin(phi)*sin(psai);
        t[0][1] = -sin(psai)*cos(phi) - cos(theta)*sin(phi)*cos(psai);
        t[0][2] = sin(theta)*sin(phi);
        
        t[1][0] = cos(psai)*sin(phi) + cos(theta)*cos(phi)*sin(psai);
        t[1][1] = -sin(psai)*sin(phi) + cos(theta)*cos(phi)*cos(psai);
        t[1][2] = -sin(theta)*cos(phi);
        
        t[2][0] = sin(psai)*sin(theta);
        t[2][1] = cos(psai)*sin(theta);
        t[2][2] = cos(theta);
        
        for (j = 1; j <= RB_A_num_per_protein; j++ ){
            for (k = 1; k <= RB_A_res_num; k++){
                R_x[i][j][k] = t[0][0]*(R_x_0[i][j][k] - R_x[i][1][1]) + t[0][1]*(R_y_0[i][j][k] - R_y[i][1][1]) + t[0][2]*(R_z_0[i][j][k] - R_z[i][1][1]) + R_x[i][1][1];
                R_y[i][j][k] = t[1][0]*(R_x_0[i][j][k] - R_x[i][1][1]) + t[1][1]*(R_y_0[i][j][k] - R_y[i][1][1]) + t[1][2]*(R_z_0[i][j][k] - R_z[i][1][1]) + R_y[i][1][1];
                R_z[i][j][k] = t[2][0]*(R_x_0[i][j][k] - R_x[i][1][1]) + t[2][1]*(R_y_0[i][j][k] - R_y[i][1][1]) + t[2][2]*(R_z_0[i][j][k] - R_z[i][1][1]) + R_z[i][1][1];
            }
        }
    }
    
    //randomly insert RB_B_C
            
            bond_num_cis = 0;
            bond_num_mono_cis = 0;
            initial_simu_time = 1;

    
        } // end else generate initial configuration
    
    
    
    /////////////////Begin main loop of Diffusion(part1)-Reaction(part2) simulation
    for (mc_time_step = initial_simu_time; mc_time_step <= simu_step; mc_time_step++){
        
        
        for (i = 1; i <= protein_A_tot_num; i++){
            for (j = 1; j <= RB_A_num_per_protein; j++){
                for (k = 1; k <= RB_A_res_num; k++){
                    R_x_new[i][j][k] = R_x[i][j][k];
                    R_y_new[i][j][k] = R_y[i][j][k];
                    R_z_new[i][j][k] = R_z[i][j][k];
                }
            }
            
            res_nei_new[i][1] = res_nei[i][1];
            res_nei_new[i][2] = res_nei[i][2];
            res_nei_new[i][3] = res_nei[i][3];
            
        }
        
        
        bond_num_mono_cis_new = bond_num_mono_cis;
        
        complex_num = 0;
        
        tot_cluster_num = 0;
        tot_proteins_in_cluster = 0;  // to culculate cluster size
        cluster_size = 0.0;
        protein_num_in_Max_Complex = 0;

        
        
        ///////////////////////////////////////////////////////////
        /////////BFS find out all the clusters containing protein_B in the system!
        
        //        cout <<  "///////////////////////////////////////// " <<'\n';
        //        cout <<  "come into matrix " <<'\n';
        
        ///////use matrix instead of vector in BFS
        
        /// initialize the matrix results to 0
        for (i = 0; i <= protein_tot_num; i++){
            for (j = 0; j <= protein_tot_num; j++){
                results[i][j] = 0;
            }
        }
        
        for (i = 1; i <= protein_tot_num; i++){
            visited[i] = 0;
            moved[i] = 0;
        }
        
        for (i = 1; i <= protein_tot_num; i++){
            if (visited[i] == 0){
                
                queue<int> qc;   //queue for complex index storage...
                
                j = 0;
                visited[i] = 1;
                qc.push(i);
                
                while(!qc.empty()){
                    
                    j = j+1;
                    results[i][j] = qc.front();    // put front element of queue into matrix results
                    q_index = qc.front();  // find out the index of front element of queue
                    qc.pop();              // pop out this element
                    
                    ////////find neighbors of this element
                    vector<int> neighbors;
                    if (q_index <= protein_A_tot_num){
                        if (res_nei_new[q_index][1] > 0) {neighbors.push_back(res_nei_new[q_index][1]);}
                        if (res_nei_new[q_index][2] > 0) {neighbors.push_back(res_nei_new[q_index][2]);}
                        if (res_nei_new[q_index][3] > 0) {neighbors.push_back(res_nei_new[q_index][3]);}
                    }
                    if (q_index > protein_A_tot_num){
                        if (res_nei_new[q_index][2] > 0) {neighbors.push_back(res_nei_new[q_index][2]);}
                        if (res_nei_new[q_index][3] > 0) {neighbors.push_back(res_nei_new[q_index][3]);}
                        if (res_nei_new[q_index][4] > 0) {neighbors.push_back(res_nei_new[q_index][4]);}
                    }
                    
                    ////////iterate neighbors
                    for (std::vector<int>::iterator it = neighbors.begin(); it!=neighbors.end(); ++it){
                        if (visited[*it] == 0){        // if not visited, put in queue
                            visited[*it] = 1;          // mark as visited
                            qc.push(*it);              // put it in queue qc
                        }
                    }
                }
            }
        }
        
        /*
         
         for (i = 1; i <= protein_tot_num; i++){
         for (j = 1; j <= protein_tot_num; j++){
         if (results[i][j] != 0){
         cout  << results[i][j]<< "  ";
         }
         }
         cout <<'\n';
         }
         cout <<  "finished results " <<'\n';
         
         */
        
        
        ///////////////// Randomly select one molecule, randomly move every molecule      START part 1 from here!
        for (iteration_mole_step = 1; iteration_mole_step <= protein_A_tot_num ; iteration_mole_step++ ){
            
            selecting_mole_index = iteration_mole_step;
            
            if (selecting_mole_index <= protein_A_tot_num
                && res_nei_new[selecting_mole_index][1] == 0
                && res_nei_new[selecting_mole_index][2] == 0
                && res_nei_new[selecting_mole_index][3] == 0){              //////// if the selected molecule is an enzyme
                
                protein_A_index = selecting_mole_index;       //make random diffusion for selected molecule  RB_A_index = 0,1,2,3... RB_A_tot_num - 1
                
                distance_amp = 2*sqrt(RB_A_D*time_step/6)*rand2();  // move this unbound molecule
                //    theta = rand2()*pai;
                phai = rand2()*2*pai;
                
                for (j = 1; j <= RB_A_num_per_protein; j++){
                    for (k = 1; k <= RB_A_res_num; k++){
                        R_x_new0[protein_A_index][j][k] = R_x[protein_A_index][j][k] + distance_amp*cos(phai);
                        R_y_new0[protein_A_index][j][k] = R_y[protein_A_index][j][k] + distance_amp*sin(phai);
                        R_z_new0[protein_A_index][j][k] = R_z[protein_A_index][j][k];
                    }
                }
                
                PB_x = cell_range_x*round(R_x_new0[protein_A_index][1][1]/cell_range_x);
                PB_y = cell_range_y*round(R_y_new0[protein_A_index][1][1]/cell_range_y);
                
                for (j = 1; j <= RB_A_num_per_protein; j++){
                    for (k = 1; k <= RB_A_res_num; k++){
                        R_x_new0[protein_A_index][j][k] = R_x_new0[protein_A_index][j][k] - PB_x;
                        R_y_new0[protein_A_index][j][k] = R_y_new0[protein_A_index][j][k] - PB_y;
                    }
                }
                
                
                ////// rotation
                theta = 0;
                phi = 0;
                psai = (2*rand2() - 1)*sqrt(RB_A_rot_D*time_step);
                
                t[0][0] = cos(psai)*cos(phi) - cos(theta)*sin(phi)*sin(psai);
                t[0][1] = -sin(psai)*cos(phi) - cos(theta)*sin(phi)*cos(psai);
                t[0][2] = sin(theta)*sin(phi);
                
                t[1][0] = cos(psai)*sin(phi) + cos(theta)*cos(phi)*sin(psai);
                t[1][1] = -sin(psai)*sin(phi) + cos(theta)*cos(phi)*cos(psai);
                t[1][2] = -sin(theta)*cos(phi);
                
                t[2][0] = sin(psai)*sin(theta);
                t[2][1] = cos(psai)*sin(theta);
                t[2][2] = cos(theta);
                
                R_x_new[protein_A_index][1][1] = R_x_new0[protein_A_index][1][1];
                R_y_new[protein_A_index][1][1] = R_y_new0[protein_A_index][1][1];
                R_z_new[protein_A_index][1][1] = R_z_new0[protein_A_index][1][1];
                
                for (j = 1; j <= RB_A_num_per_protein; j++){
                    for (k = 1; k <= RB_A_res_num; k++){
                        R_x_new[protein_A_index][j][k] = t[0][0]*(R_x_new0[protein_A_index][j][k] - R_x_new[protein_A_index][1][1]) + t[0][1]*(R_y_new0[protein_A_index][j][k] - R_y_new[protein_A_index][1][1]) + t[0][2]*(R_z_new0[protein_A_index][j][k] - R_z_new[protein_A_index][1][1]) + R_x_new[protein_A_index][1][1];
                        R_y_new[protein_A_index][j][k] = t[1][0]*(R_x_new0[protein_A_index][j][k] - R_x_new[protein_A_index][1][1]) + t[1][1]*(R_y_new0[protein_A_index][j][k] - R_y_new[protein_A_index][1][1]) + t[1][2]*(R_z_new0[protein_A_index][j][k] - R_z_new[protein_A_index][1][1]) + R_y_new[protein_A_index][1][1];
                        R_z_new[protein_A_index][j][k] = t[2][0]*(R_x_new0[protein_A_index][j][k] - R_x_new[protein_A_index][1][1]) + t[2][1]*(R_y_new0[protein_A_index][j][k] - R_y_new[protein_A_index][1][1]) + t[2][2]*(R_z_new0[protein_A_index][j][k] - R_z_new[protein_A_index][1][1]) + R_z_new[protein_A_index][1][1];
                    }
                }
                
                
                ///////check collision
                collision_flag = 0;
                
                for (i = 1; i <= protein_A_tot_num; i++){      ///check overlap between RB_A and all the other RB_A
                    for (j = 1; j <= RB_A_num_per_protein; j++){
                        for (k = 1; k <= RB_A_num_per_protein; k++){
                            if (protein_A_index != i){
                                dist=sqrt( (R_x_new[i][j][1]-R_x_new[protein_A_index][k][1])*(R_x_new[i][j][1]-R_x_new[protein_A_index][k][1])
                                          +(R_y_new[i][j][1]-R_y_new[protein_A_index][k][1])*(R_y_new[i][j][1]-R_y_new[protein_A_index][k][1])
                                          +(R_z_new[i][j][1]-R_z_new[protein_A_index][k][1])*(R_z_new[i][j][1]-R_z_new[protein_A_index][k][1])
                                          );
                                if (dist < RB_A_radius + RB_A_radius) {
                                    collision_flag = 1;
                                }
                            }
                        }
                    }
                }
                
                
                if (collision_flag == 1){
                    for (j = 1; j <= RB_A_num_per_protein; j++){
                        for (k = 1; k <= RB_A_res_num; k++){
                            R_x_new[protein_A_index][j][k] = R_x[protein_A_index][j][k];
                            R_y_new[protein_A_index][j][k] = R_y[protein_A_index][j][k];
                            R_z_new[protein_A_index][j][k] = R_z[protein_A_index][j][k];
                        }
                    }
                }
                
                
            } //end -- selected molecule is an enzyme, all RB_A_index
            
            ///////////////////////////////////////////////////
            ///////////////////////////////////////////////////start protein_B
            
            //end -- selected molecule is an enzyme, all RB_A_index
            
            
            
            
            //// start to traversal each complex
            
            if (selecting_mole_index <= protein_tot_num ){
                
                complex_index = selecting_mole_index;
                complex_size = 0;
                protein_A_num_inComplex = 0;    // prepare to find out how many protein_A in complex
                
                protein_A_index = 0;

                
                
                for ( complex_sub_index= 1; complex_sub_index <= protein_tot_num; complex_sub_index++){
                    if (results[complex_index][complex_sub_index] == 0){break;}
                    if (results[complex_index][complex_sub_index] <= protein_A_tot_num) {
                        protein_A_num_inComplex++;
                        protein_A_index = results[complex_index][complex_sub_index];
                    }
                    
                    complex_size++;
                }
                
                if (complex_size > protein_num_in_Max_Complex){
                    protein_num_in_Max_Complex = complex_size;
                }
                
                
                
                ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                if (complex_size > 1) {
                    
                    tot_cluster_num = tot_cluster_num + 1;
                    tot_proteins_in_cluster = tot_proteins_in_cluster + complex_size;
                    
                    
                    // to calculate protein_A_cluster number....... double check
                    if (protein_A_index != 0){
                        complex_num = complex_num + 1;
                    }
                    
                    /// dimer stop moving!!
                    ///make cluster move
                        
                        PB_x = 0;
                        PB_y = 0;
                        PB_z = 0;
                        
                        distance_amp = 2*sqrt((bond_D/complex_size)*time_step/6)*rand2();  // move this bond together
                        phai = rand2()*2*pai;
                        
                        for ( complex_sub_index= 1; complex_sub_index <= protein_tot_num; complex_sub_index++){
                            if (results[complex_index][complex_sub_index] == 0){break;}
                            
                            //////////////////////////////////////////////////////////////////////////////
                            //////////////////////////////////////////////////////////////////////////////////////////////////////
                            
                            if (results[complex_index][complex_sub_index] <= protein_A_tot_num) {  ///////move protein_A
                                
                                protein_A_index = results[complex_index][complex_sub_index];
                                
                                for (j = 1; j <= RB_A_num_per_protein; j++){
                                    for (k = 1; k <= RB_A_res_num; k++){
                                        R_x_new0[protein_A_index][j][k] = R_x[protein_A_index][j][k] + distance_amp*cos(phai);
                                        R_y_new0[protein_A_index][j][k] = R_y[protein_A_index][j][k] + distance_amp*sin(phai);
                                        R_z_new0[protein_A_index][j][k] = R_z[protein_A_index][j][k];
                                    }
                                }
                                PB_x = PB_x + R_x_new0[protein_A_index][1][1];
                                PB_y = PB_y + R_y_new0[protein_A_index][1][1];
                            }
                            
                            ////////////////////////////////////////////////////////////////////////////////////
                            //////////////////////////////////////////////////////////////////////////////////////////////////////
                            
                            
                            //////////////////////////////////////////////////////////////////////////////////////////////////////
                            //////////////////////////////////////////////////////////////////////////////////////////////////////
                            
                            
                            //////////////////////////////////////////////////////////////////////////////////////////////////////
                            //////////////////////////////////////////////////////////////////////////////////////////////////////
                            
                        }// end traversal all A B C D, and move them together
                        
                        PB_x = cell_range_x*round(PB_x/(protein_A_num_inComplex)/cell_range_x);
                        PB_y = cell_range_y*round(PB_y/(protein_A_num_inComplex)/cell_range_y);
                        
                        cm1_a_x = 0;
                        cm1_a_y = 0;
                        cm1_a_z = 0;
                        
                        for ( complex_sub_index= 1; complex_sub_index <= protein_tot_num; complex_sub_index++){
                            if (results[complex_index][complex_sub_index] == 0){break;}
                            
                            //////////////////////////////////////////////////////////////////////////////////////////////////////
                            //////////////////////////////////////////////////////////////////////////////////////////////////////
                            if ( results[complex_index][complex_sub_index] <= protein_A_tot_num) {
                                protein_A_index = results[complex_index][complex_sub_index];
                                
                                for (j = 1; j <= RB_A_num_per_protein; j++){
                                    for (k = 1; k <= RB_A_res_num; k++){
                                        R_x_new0[protein_A_index][j][k] = R_x_new0[protein_A_index][j][k] - PB_x;
                                        R_y_new0[protein_A_index][j][k] = R_y_new0[protein_A_index][j][k] - PB_y;
                                    }
                                }
                                
                                for (j = 1; j <= RB_A_num_per_protein; j++){
                                    cm1_a_x = cm1_a_x + R_x_new0[protein_A_index][j][1];
                                    cm1_a_y = cm1_a_y + R_y_new0[protein_A_index][j][1];
                                    cm1_a_z = cm1_a_z + R_z_new0[protein_A_index][j][1];
                                }
                            }
                            
                            //////////////////////////////////////////////////////////////////////////////////////////////////////
                            
                            
                            //////////////////////////////////////////////////////////////////////////////////////////////////////
                            /////////////////////////////////////////////////////////////////////////////////////////////////////
                            
                            //////////////////////////////////////////////////////////////////////////////////////////////////////
                            //////////////////////////////////////////////////////////////////////////////////////////////////////
                        }  // end PBC checking
                        
                        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                        
                        //find rotation center//center of mass of complex
                        cm1_a_x = cm1_a_x / (RB_A_num_per_protein*protein_A_num_inComplex);
                        cm1_a_y = cm1_a_y / (RB_A_num_per_protein*protein_A_num_inComplex);
                        cm1_a_z = cm1_a_z / (RB_A_num_per_protein*protein_A_num_inComplex);
                        
                        ////// prepare for rotational move
                        
                        theta = 0;
                        phi = 0;
                        psai = (2*rand2() - 1)*sqrt((bond_rot_D/exp(complex_size))*time_step);
                        
                        t[0][0] = cos(psai)*cos(phi) - cos(theta)*sin(phi)*sin(psai);
                        t[0][1] = -sin(psai)*cos(phi) - cos(theta)*sin(phi)*cos(psai);
                        t[0][2] = sin(theta)*sin(phi);
                        
                        t[1][0] = cos(psai)*sin(phi) + cos(theta)*cos(phi)*sin(psai);
                        t[1][1] = -sin(psai)*sin(phi) + cos(theta)*cos(phi)*cos(psai);
                        t[1][2] = -sin(theta)*cos(phi);
                        
                        t[2][0] = sin(psai)*sin(theta);
                        t[2][1] = cos(psai)*sin(theta);
                        t[2][2] = cos(theta);
                        
                        for ( complex_sub_index= 1; complex_sub_index <= protein_tot_num; complex_sub_index++){
                            if (results[complex_index][complex_sub_index] == 0){break;}
                            
                            if (results[complex_index][complex_sub_index] <= protein_A_tot_num) {
                                protein_A_index = results[complex_index][complex_sub_index];
                                for (j = 1; j <= RB_A_num_per_protein; j++){
                                    for (k = 1; k <= RB_A_res_num; k++){
                                        R_x_new[protein_A_index][j][k] = t[0][0]*(R_x_new0[protein_A_index][j][k] - cm1_a_x) + t[0][1]*(R_y_new0[protein_A_index][j][k] - cm1_a_y) + t[0][2]*(R_z_new0[protein_A_index][j][k] - cm1_a_z) + cm1_a_x;
                                        R_y_new[protein_A_index][j][k] = t[1][0]*(R_x_new0[protein_A_index][j][k] - cm1_a_x) + t[1][1]*(R_y_new0[protein_A_index][j][k] - cm1_a_y) + t[1][2]*(R_z_new0[protein_A_index][j][k] - cm1_a_z) + cm1_a_y;
                                        R_z_new[protein_A_index][j][k] = t[2][0]*(R_x_new0[protein_A_index][j][k] - cm1_a_x) + t[2][1]*(R_y_new0[protein_A_index][j][k] - cm1_a_y) + t[2][2]*(R_z_new0[protein_A_index][j][k] - cm1_a_z) + cm1_a_z;
                                    }
                                }
                            }
                            
                            
                        }  // end rotation
                        ///////////////////////////////////////////////////////////////////////////////////
                        /////////////finished translational and rotational move
                        
                        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                        ////////// Align Protein_B with Protein_A
                        
                        
                        
                        
                        
                        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                        /////check collision
                        collision_flag = 0;
                        
                        for ( complex_sub_index= 1; complex_sub_index <= protein_tot_num; complex_sub_index++){
                            if (results[complex_index][complex_sub_index] == 0){break;}
                            
                            if ( results[complex_index][complex_sub_index] <= protein_A_tot_num) {
                                protein_A_index = results[complex_index][complex_sub_index];
                                
                                for (i = 1; i <= protein_A_tot_num; i++){      ///check overlap between RB_A and all the other RB_A
                                    for (j = 1; j <= RB_A_num_per_protein; j++){
                                        for (k = 1; k <= RB_A_num_per_protein; k++){
                                            if (protein_A_index != i){
                                                dist=sqrt( (R_x_new[i][j][1]-R_x_new[protein_A_index][k][1])*(R_x_new[i][j][1]-R_x_new[protein_A_index][k][1])
                                                          +(R_y_new[i][j][1]-R_y_new[protein_A_index][k][1])*(R_y_new[i][j][1]-R_y_new[protein_A_index][k][1])
                                                          +(R_z_new[i][j][1]-R_z_new[protein_A_index][k][1])*(R_z_new[i][j][1]-R_z_new[protein_A_index][k][1])
                                                          );
                                                if (dist < RB_A_radius + RB_A_radius) {
                                                    collision_flag = 1;
                                                }
                                            }
                                        }
                                    }
                                }
                                
                            }
                            
                            
                            
                        }
                    
                }  ////////end  check collision, and next step : put them back when flag = 1
                
                
                if (collision_flag == 1){
                    for ( complex_sub_index= 1; complex_sub_index <= protein_tot_num; complex_sub_index++){
                        if (results[complex_index][complex_sub_index] == 0){break;}
                        
                        
                        if ( results[complex_index][complex_sub_index] <= protein_A_tot_num) {
                            protein_A_index = results[complex_index][complex_sub_index];
                            
                            for (j = 1; j <= RB_A_num_per_protein; j++){
                                for (k = 1; k <= RB_A_res_num; k++){
                                    R_x_new[protein_A_index][j][k] = R_x[protein_A_index][j][k];
                                    R_y_new[protein_A_index][j][k] = R_y[protein_A_index][j][k];
                                    R_z_new[protein_A_index][j][k] = R_z[protein_A_index][j][k];
                                }
                            }
                        }
                        
                    }
                }/// put the collision molecules back when flag = 1
                
                
                /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                
                
            }  // end traversal each complex
            
        } // end iteration_mole_step, end diffusion! end part1 !
        
        
        
        /////////////// START Reaction, start part2
        
        // (1) RB_A and RB_B associate into a bond
        
        // (1) RB_A and RB_B associate into a bond
        
        // (1) RB_A and RB_D associate into a bond
        
        
        /////  type (1) protein_B and protein_C  cis interaction occurs !!
        for (i = 1; i <= protein_A_tot_num; i++){
            for (j = 1; j <= protein_A_tot_num ; j++){
                if (   i != j && res_nei_new[i][3] == 0 && res_nei_new[j][2] == 0){
                    dist=sqrt( (R_x_new[j][1][2]-R_x_new[i][1][3])*(R_x_new[j][1][2]-R_x_new[i][1][3])
                              +(R_y_new[j][1][2]-R_y_new[i][1][3])*(R_y_new[j][1][2]-R_y_new[i][1][3])
                              +(R_z_new[j][1][2]-R_z_new[i][1][3])*(R_z_new[j][1][2]-R_z_new[i][1][3])
                              );
                    if (dist < cis_dist_cutoff){
                        
                        theta_ot = 0;
                        point_x[0] = R_x_new[i][1][1] - R_x_new[i][1][3];
                        point_y[0] = R_y_new[i][1][1] - R_y_new[i][1][3];
                        point_z[0] = R_z_new[i][1][1] - R_z_new[i][1][3];
                        point_x[1] = 0;
                        point_y[1] = 0;
                        point_z[1] = 0;
                        point_x[2] = R_x_new[j][1][1] - R_x_new[j][1][2];
                        point_y[2] = R_y_new[j][1][1] - R_y_new[j][1][2];
                        point_z[2] = R_z_new[j][1][1] - R_z_new[j][1][2];
                        theta_ot2 =  gettheta ( point_x,  point_y,  point_z,  theta_ot);
                        //     cout << "cis theta_ot2 = " << theta_ot2<<'\n';
                        
                        
                        
                        if (abs(theta_ot2 - 180) < cis_thetaot_cutoff){
                            ///// mark mistake!!  angle - 180!!
                            
                            Prob_Ass = cis_Ass_Rate*time_step;
                            prob = rand2();
                            
                            if ((prob < Prob_Ass)) {
                                bond_num_mono_cis_new = bond_num_mono_cis_new + 1;
                                
                                res_nei_new[j][2] = i;
                                res_nei_new[i][3] = j;
                                
                                //            cout << "cis i = " << bond_nb_ctg_pro_idx_new[bond_num_new][3]<< "  cis j = " << bond_nb_ctg_pro_idx_new[bond_num_new][4]<<'\n';
                            }
                        }
                    }
                }
            }
        }
        
        
        
        for (i = 1; i <= protein_A_tot_num; i++){
            
            if (res_nei_new[i][3] != 0){
                selected_pro_A = i;
                selected_pro_A2 = res_nei_new[i][3];        //mistake!! it's not res_nei_new[i][2]
                
                Prob_Diss = cis_Diss_Rate * time_step;
                prob = rand2();
                if (prob < Prob_Diss){
                    
                    res_nei_new[selected_pro_A][3] = 0;
                    res_nei_new[selected_pro_A2][2] = 0;
                    
                    bond_num_mono_cis_new = bond_num_mono_cis_new - 1;
                }
            }
        }
        
        
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /////////////////////////
        
        
        
        ///////update coordinates for all molecules
        
        for (i = 1; i <= protein_A_tot_num; i++){
            for (j = 1; j <= RB_A_num_per_protein; j++){
                for (k = 1; k <= RB_A_res_num; k++){
                    R_x[i][j][k] = R_x_new[i][j][k];
                    R_y[i][j][k] = R_y_new[i][j][k];
                    R_z[i][j][k] = R_z_new[i][j][k];
                }
            }
            
            res_nei[i][1] = res_nei_new[i][1];
            res_nei[i][2] = res_nei_new[i][2];
            res_nei[i][3] = res_nei_new[i][3];
            
        }
        
        bond_num_mono_cis = bond_num_mono_cis_new;
        
        if (tot_cluster_num != 0){
            cluster_size = (double)tot_proteins_in_cluster/tot_cluster_num;
        }
        
        ///////////Check Point file:
        if (mc_time_step % 1000 == 0){
            std::ofstream check_point ("position.cpt", std::ofstream::trunc);
            check_point.setf(ios::fixed, ios::floatfield);  // make output numbers look neat
            check_point << setprecision(3);
            
            for (i = 1; i <= protein_A_tot_num; i++){
                for (j = 1; j <= RB_A_num_per_protein; j++){
                    for (k = 1; k <= RB_A_res_num; k++){
                        check_point <<setw(10)<< R_x[i][j][k] <<setw(10)<< R_y[i][j][k] <<setw(10)<< R_z[i][j][k] <<'\n';
                    }
                }
                check_point <<setw(8)<< res_nei_new[i][1];
                check_point <<setw(8)<< res_nei_new[i][2];
                check_point <<setw(8)<< res_nei_new[i][3]<<'\n';
            }
            
            check_point << bond_num_mono_cis_new<<'\n';
            check_point << mc_time_step<<'\n';
            
            check_point.close();
        }
        
        
        if (mc_time_step % 1000 == 0){
            std::ofstream bond ("bond.dat", std::ofstream::app);
            bond.setf(ios::fixed, ios::floatfield);  // make output numbers look neat
            bond << setprecision(3);
            bond <<setw(15)<< mc_time_step*time_step<<setw(5)<< bond_num_mono_cis <<setw(5)<< tot_cluster_num <<setw(10) << cluster_size<< setw(10) << protein_num_in_Max_Complex<<'\n';
            bond.close();
        }
        
        
        
        
        if (mc_time_step % 1000 == 0){
            std::ofstream ofs ("test.gro", std::ofstream::app); //open file and append to write
            ofs.setf(ios::fixed, ios::floatfield);  // make output numbers look neat
            ofs << setprecision(3);
            ofs << "Hello Gro!" << ", t="<< mc_time_step*time_step <<'\n';
            //ofs << protein_A_tot_num*RB_A_num_per_protein + protein_B_tot_num*(RB_B_num_per_protein - 1) <<'\n';
            ofs << protein_A_tot_num*RB_A_num_per_protein<<'\n';
            
            for (i=1; i<= protein_A_tot_num; i++){
                for (j = 1; j <= RB_A_num_per_protein; j++){
                    ofs <<setw(5)<<i<<"ALA" <<setw(7)<< "CA" << setw(5)<< i <<setw(8)<< R_x[i][j][1]/10 <<setw(8)<< R_y[i][j][1]/10 <<setw(8)<< R_z[i][j][1]/10 <<'\n';
                    //    ofs <<setw(5)<<i<<"VAL" <<setw(7)<< "CA" << setw(5)<< i <<setw(8)<< R_x[i][j][2]/10 <<setw(8)<< R_y[i][j][2]/10 <<setw(8)<< R_z[i][j][2]/10 <<'\n';
                    //    ofs <<setw(5)<<i<<"VAL" <<setw(7)<< "CA" << setw(5)<< i <<setw(8)<< R_x[i][j][3]/10 <<setw(8)<< R_y[i][j][3]/10 <<setw(8)<< R_z[i][j][3]/10 <<'\n';
                    //    ofs <<setw(5)<<i<<"LEU" <<setw(7)<< "CA" << setw(5)<< i <<setw(8)<< R_x[i][j][4]/10 <<setw(8)<< R_y[i][j][4]/10 <<setw(8)<< R_z[i][j][4]/10 <<'\n';
                    //    ofs <<setw(5)<<i<<"VAL" <<setw(7)<< "CA" << setw(5)<< i <<setw(8)<< R_x[i][j][5]/10 <<setw(8)<< R_y[i][j][5]/10 <<setw(8)<< R_z[i][j][5]/10 <<'\n';
                    //          cout << " Ax = " << R_x[i][j][4] << " Ay = " << R_y[i][j][4] <<" Az = " << R_z[i][j][4] <<endl;
                    
                }
            }
            ofs << setw(8)<< cell_range_x/10 << setw(12)<< cell_range_y/10 << setw(12)<< cell_range_z/10<<'\n';
            ofs.close();
            
        } // end writing coordinates every 10 time_step
        
        
        
        if (mc_time_step % 1000 == 0){
            std::ofstream cluster ("cluster.log", std::ofstream::app); //open file and append to write
            cluster << "Hello Cluster!" << ", t="<< mc_time_step*time_step <<'\n';
            for (i = 1; i <= protein_tot_num; i++){
                for (j = 1; j <= protein_tot_num; j++){
                    if (results[i][j] != 0){
                        cluster  << results[i][j]<< "  ";
                    }
                }
                cluster <<'\n';
            }
            
            cluster.close();
            
        } // end writing coordinates every 10 time_step
        
        
    } //end mc_time_step, end diffusion-reaction loop
} // end main loop



double rand2(){
    std::mt19937_64 rng;
    // initialize the random number generator with time-dependent seed
    uint64_t timeSeed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::seed_seq ss{uint32_t(timeSeed & 0xffffffff), uint32_t(timeSeed>>32)};
    rng.seed(ss);
    // initialize a uniform distribution between 0 and 1
    std::uniform_real_distribution<double> unif(0, 1);
    // ready to generate random numbers
    
    double currentRandomNumber = unif(rng);
    return currentRandomNumber;
    //    std::cout << currentRandomNumber << std::endl;
}


double gettheta (double point_x[3], double point_y[3], double point_z[3], double test_theta){
    double lx[2],ly[2],lz[2],lr[2];
    double conv, doth1, doth2;
    int in;
    
    for ( in=0; in < 2; in++){
        lx[in] = 0;
        ly[in] = 0;
        lz[in] = 0;
        lr[in] = 0;
    }
    
    lx[0]= point_x[1] - point_x[0];
    ly[0]= point_y[1] - point_y[0];
    lz[0]= point_z[1] - point_z[0];
    lr[0]= sqrt(lx[0]*lx[0]+ly[0]*ly[0]+lz[0]*lz[0]);
    
    lx[1]= point_x[2] - point_x[1];
    ly[1]= point_y[2] - point_y[1];
    lz[1]= point_z[2] - point_z[1];
    lr[1]= sqrt(lx[1]*lx[1]+ly[1]*ly[1]+lz[1]*lz[1]);
    
    test_theta = 0;
    
    conv = 180/3.14159;
    
    doth1 = -(lx[1]*lx[0] + ly[0]*ly[1] + lz[0]*lz[1]);
    doth2 = doth1/(lr[1]*lr[0]);
    if (doth2 > 1){
        doth2 = 1;
    }
    if (doth2 < -1){
        doth2 = -1;
    }
    
    test_theta = acos(doth2)* conv;
    return test_theta;
}

bool AreSame(double a, double b)
{
    return fabs(a - b) < EPSILON;
}


///////// put z coordinates into right value

//angle between  RB_B [j][1][1] -------> RB_A[i][3][1] and [0][0] ---> [0][1] in x-y plane
//   x2 = R_x_new[i][2][1] - R_x_new[j][1][1];
//   y2 = R_y_new[i][2][1] - R_y_new[j][1][1];
//     x1 = 0;
//     y1 = 1;
//   dot = x1*x2 + y1*y2;    // dot product between [x1, y1] and [x2, y2]
//   det = x1*y2 - y1*x2;    // determinant
//   angle = atan2(det, dot)   // this is wrong! because the range is -180 to 180, but we need 0 to 360
// atan2 usually is in the range [-180°,180°]. To get [0°,360°] without a case distinction, one can replace atan2(y,x) with atan2(-y,-x) + 180°
// from point 1 counterclokewise to point 2 !!

//angle = atan2((R_x_new[protein_B_index][2][1] - R_x_new[protein_B_index][1][1]), (R_y_new[protein_B_index][2][1] - R_y_new[protein_B_index][1][1])) + pai;

// for 2nd case, angle between










