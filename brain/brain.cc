
#include <iostream>
#include <thread>
#include <math.h>
#include <unistd.h>
#include <algorithm>
#include <atomic>

#include "robot.hh"
#include "viz.hh"

using std::cout;
using std::endl;

float res = 0.25;
float x_offset = 26 / res;
float y_offset = 26 / res;
float x_res = 52 / res;
float y_res = 52 / res;
float log_odd_occ = 0.05;
float log_odd_free = 0.95;
float count = 0.00;
float observed_x = 0.00;
float observed_y = 0.00;
bool wallfound = false;
bool transition = false;
std::atomic<int> estimate_x {0};
std::atomic<int> estimate_y {0};


float grid[208][208];

// Assumptions :
// 1) Line is drawn from left to right.
// 2) x1 < x2 and y1 < y2
// 3) Slope of the line is between 0 and 1.
//    We draw a line from lower left to upper
//    right.
// function for line generation
void bresenham(int x1, int y1, int x2, int y2, int tgt_x, int tgt_y)
{
    int m_new = 2 * (x2 - x1);
    int slope_error_new = m_new - (y2 - y1);
    for (int x = x1, y = y1; y <= y2; y++)
    {
        if (x != tgt_x || y != tgt_y) {
            x = clamp(0, x, 207);
            y = clamp(0, y, 207);
            //cout << "MISS @ " << x << ", " << y << endl;
            grid[x][y] -= log_odd_free;
        }

        // Add slope to increment angle formed
        slope_error_new += m_new;

        // Slope error reached limit, time to
        // increment y and update slope error.
        if (slope_error_new >= 0)
        {
            x++;
            slope_error_new  -= 2 * (y2 - y1);
        }
   }
}

void
callback(Robot* robot)
{
    std::vector<float> angles;

    int x = estimate_x;
    int y = estimate_y;

    for (auto hit : robot->ranges) {
        if (hit.range <= 2) {
            // get hit info for the occupancy grid
            angles.push_back(hit.angle);

            int angle = hit.angle + (M_PI / 2.0);
            float dx = 0.5 * hit.range * cos(angle);
            float dy = 0.5 * hit.range * sin(angle);
            float abs_x = x + dx;
            float abs_y = y + dy;
            abs_x = abs_x / res;
            abs_y = abs_y / res;
            float adj_x = abs(abs_x - x_offset);
            float adj_y = abs(abs_y - y_offset);
            adj_x = clamp(0, adj_x, 207);
            adj_y = clamp(0, adj_y, 207);

            float start_x = abs(x/res - x_offset);
            float start_y = abs(y/res - y_offset);
            start_x = ceil(start_x);
            start_y = ceil(start_y);
            start_x = clamp(0, start_x, 207);
            start_y = clamp(0, start_y, 207);

            //cout << "HIT: (" << (int)adj_x << ", " << (int)adj_y << ") " << endl;
            grid[(int)adj_x][(int)adj_y] += log_odd_occ;

            // update misses
            if (start_x > adj_x && start_y > adj_y) {
                bresenham(adj_x, adj_y, start_x, start_y, adj_x, adj_y);    
            } else {
                bresenham(start_x, start_y, adj_x, adj_y, adj_x, adj_y);
            }

        } 
    }

    int size = angles.size();
    while (size < 7) {
        //cout << "Size: " << size << endl;
        // update all estimated points based on misses
        std::vector<float> angles_raw = {2.35619, 1.57079, 0.785397, 0, -0.785397, -1.57079, -2.35619};
        for (float angle : angles_raw) {
            bool found = false;
            for (float angle_hit : angles) {
               if (abs(angle_hit - angle) < 0.1) {
                    found = true;
               } 
            }
            if (!found) {
                //cout << "Updating a miss on " << angle << endl;
                int angle = angle + (M_PI / 2.0);
                float dx = 0.5 * 2 * cos(angle);
                float dy = 0.5 * 2 * sin(angle);
                int abs_x = round(x + dx);
                int abs_y = round(y + dy);
                abs_x = abs_x / res;
                abs_y = abs_y / res;
                int adj_x = abs(abs_x - x_offset);
                int adj_y = abs(abs_y - y_offset);

                int start_x = abs(x/res - x_offset);
                int start_y = abs(y/res - y_offset);
            
                if (start_x > adj_x && start_y > adj_y) {
                    bresenham(adj_x, adj_y, start_x, start_y, -1, -1);    
                } else {
                    bresenham(start_x, start_y, adj_x, adj_y, -1, -1);
                }   
            }
        }
        size++;
    }

    int scenario[7] = {false, false, false, false, false, false, false};
    int close[7] = {false, false, false, false, false, false, false};
    for (auto hit : robot->ranges) {

        if (hit.range < 1.5) {
            if (abs(hit.angle) < 0.3) {
                cout << hit.angle << endl;
                close[0] = true;
            }
            if (abs(hit.angle - 0.78539) < 0.1) {
                close[1] = true;
            }
            if (abs(hit.angle + 0.78539) < 0.1) {
                close[2] = true;
            }
            if (abs(hit.angle - 1.57) < 0.1) {
                close[3] = true;
            }
            if (abs(hit.angle + 1.57) < 0.1) {
                close[4] = true;
            }
            if (abs(hit.angle - 2.35619) < 0.1) {
                close[5] = true;
            }
            if (abs(hit.angle + 2.35619) < 0.1) {
                close[6] = true;
            }
        } 
        
        if (hit.range < 2) {
            //cout << hit.angle << endl;
            if (abs(hit.angle) < 0.3) {
                scenario[0] = true;
            }
            if (abs(hit.angle - 0.78539) < 0.1) {
                scenario[1] = true;
            }
            if (abs(hit.angle + 0.78539) < 0.1) {
                scenario[2] = true;
            }
            if (abs(hit.angle - 1.57) < 0.1) {
                scenario[3] = true;
            }
            if (abs(hit.angle + 1.57) < 0.1) {
                scenario[4] = true;
            }
            if (abs(hit.angle - 2.35619) < 0.1) {
                scenario[5] = true;
            }
            if (abs(hit.angle + 2.35619) < 0.1) {
                scenario[6] = true;
            }
        }
    }

    if (close[0]) {
        robot->set_vel(2.0, -2.0);
        cout << "Close front" << endl;
        return;
    } else if (close[1]) {
        robot->set_vel(2.0, -2.0);
        cout << "Close on left" << endl;
        return;
    } else if (close[1] && close[3]) {
        robot->set_vel(1.5, -1.5);
        cout << "Close on left 2" << endl;
        return;
    }

    if (wallfound) {
        return;
    }

    if (scenario[0]) {
        robot->set_vel(2.0, -2.0);
        cout << "Case 0" << endl;
        return;
    } else if (scenario[0] && scenario[1] && scenario[3] && scenario[5]) {
        robot->set_vel(2.0, -2.0);
        cout << "All left" << endl;
        return;  
    } else if (scenario[1]) {
        robot->set_vel(1.0, 1.0);
        wallfound = true;
        cout << "Case 1" << endl;
        return;
    } else if (scenario[3] && !scenario[1] && !scenario[0]) {
        if (robot->pos_t > 0) {
            robot->set_vel(0.0, 2.0);
        } else {
            robot->set_vel(2.0, 0.0);
        }
        cout << "Case 2.1" << endl;
        return;
    } else if (scenario[2]) {
        robot->set_vel(2.0, -2.0);
        cout << "Case 2.2" << endl;
        return;
    } else if (scenario[3] || scenario[4]) {
        robot->set_vel(1.0, 1.0);
        cout << "Case 3.1" << endl;
        return;
    } else if (scenario[5]) {
        robot->set_vel(0, 3.0);
        cout << "Case 5" << endl;
        return;
    } else if (scenario[6]) {
        robot->set_vel(2.0, 0);
        cout << "Case 6" << endl;
        return;
    } else {
        wallfound = false;
        cout << "No hits" << endl;
        robot->set_vel(0.0, 4.0);
        return;
    }

}

void
robot_thread(Robot* robot)
{
    robot->do_stuff();
}

void
draw_thread() 
{
    while (true) {
        std::this_thread::sleep_for (std::chrono::seconds(5));
        clear();
        //cout << "Drawing grid" << endl;
        for (int i = 0; i < 208; i++) {
            for (int j = 0; j < 208; j++) {
                if (grid[i][j] >= 1.2) {
                    //cout << "Drawn: " << i << ", " << j << " Confidence: " << grid[i][j] << endl;
                    draw_index(j, i);
                }
            }
        }
    }
}

void 
pos_thread(Robot* robot)
{
    while (true) {
        if (count < 2000) {
            count++;
            observed_x += robot->pos_x;
            observed_y += robot->pos_y;
        } else {
            estimate_x = observed_x / count;
            estimate_y = observed_y / count;
            count = 0;
            observed_x = 0;
            observed_y = 0;
        }
    }
}

int
main(int argc, char* argv[])
{
    cout << "making robot" << endl;
    Robot robot(argc, argv, callback);
    std::thread rthr(robot_thread, &robot);
    std::thread dthr(draw_thread);
    std::thread pthr(pos_thread, &robot);

    return viz_run(argc, argv);
}
