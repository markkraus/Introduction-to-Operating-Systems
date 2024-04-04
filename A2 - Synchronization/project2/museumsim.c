#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#include "museumsim.h"

struct shared_data
{
  pthread_mutex_t mutex;

  pthread_cond_t guide_can_leave;   // Guide waits: Signaled when a visitor leaves museum
  pthread_cond_t visitor_arrived;   // Guide waits: Signaled when a visitor arrives outside the museum
  pthread_cond_t guide_can_enter;   // Guide waits: Signaled when a guide leaves museum
  pthread_cond_t visitor_can_enter; // Visitor waits: Signaled when a guide is admitting a visitor

  int tour_size;                // Total tour size of a given run
  int curr_tour_size;           // Size of a given tour
  int visitors_waiting_outside; // Guests that have arrived outside the museum
  int guides_inside;            // Guides that have entered the museum       
  int visitors_inside;          // Guests taht are touring the museum 
  int guide_leaving;            // Flagged when a guide is in the process of leaving the museum
};

static struct shared_data shared;

/**
 * Set up the shared variables for your implementation.
 *
 * `museum_init` will be called before any threads of the simulation
 * are spawned.
 */
void museum_init(int num_guides, int num_visitors)
{
  pthread_mutex_init(&shared.mutex, NULL);
  
  pthread_cond_init(&shared.guide_can_leave, NULL);
  pthread_cond_init(&shared.visitor_arrived, NULL);
  pthread_cond_init(&shared.guide_can_enter, NULL);
  pthread_cond_init(&shared.visitor_can_enter, NULL);

  shared.tour_size = num_visitors;
  shared.curr_tour_size = num_visitors + 1;
  shared.visitors_waiting_outside = 0;
  shared.guides_inside = 0;
  shared.visitors_inside = 0;
  shared.guide_leaving = 0;
}

/**
 * Tear down the shared variables for your implementation.
 *
 * `museum_destroy` will be called after all threads of the simulation
 * are done executing.
 */
void museum_destroy()
{
  pthread_mutex_destroy(&shared.mutex);
  pthread_cond_destroy(&shared.guide_can_leave);
  pthread_cond_destroy(&shared.visitor_arrived);
  pthread_cond_destroy(&shared.guide_can_enter);
  pthread_cond_destroy(&shared.visitor_can_enter);
}

/**
 * Implements the visitor arrival, touring, and leaving sequence.
 */
void visitor(int id)
{
  // Visitor arrival
  visitor_arrives(id);
  pthread_mutex_lock(&shared.mutex);
  {
    // Claim one of the tour slots
    int ticket_num = shared.tour_size;
    shared.tour_size--;
    shared.visitors_waiting_outside++;
    pthread_cond_broadcast(&shared.visitor_arrived); // Let guide know visitor is here
    
    while (ticket_num < shared.curr_tour_size) {
      // Wait for group of visitors and tour guide
      pthread_cond_wait(&shared.visitor_can_enter, &shared.mutex); 
    }
  }
  pthread_mutex_unlock(&shared.mutex);

  // Visitor touring
  visitor_tours(id);

  // Visitor leaving
  pthread_mutex_lock(&shared.mutex);
  {
    shared.visitors_inside--; 
    visitor_leaves(id);
    pthread_cond_broadcast(&shared.guide_can_leave); // Let guide know visitor left
  }
  pthread_mutex_unlock(&shared.mutex);
}

/**
 * Implements the guide arrival, entering, admitting, and leaving sequence.
 */
void guide(int id)
{
  // Guide arrival
  pthread_mutex_lock(&shared.mutex);
  {
    guide_arrives(id);
    while (shared.guide_leaving != 0 || shared.guides_inside == GUIDES_ALLOWED_INSIDE) {
      // Wait until guide inside has left
      pthread_cond_wait(&shared.guide_can_enter, &shared.mutex); 
    }

    // Guide entering
    guide_enters(id);
    shared.guides_inside++;

    // Guide admitting
    int admitted_visitors = 0; // number of visitors guide has allowed into the museum
    while (admitted_visitors < VISITORS_PER_GUIDE) {
      while (shared.visitors_waiting_outside == 0) {
        // Wait for visitors to enter the museum
        pthread_cond_wait(&shared.visitor_arrived, &shared.mutex);
      }

      // Admit the visitor
      shared.visitors_waiting_outside--; 
      shared.curr_tour_size--;
      shared.visitors_inside++;
      admitted_visitors++;
      pthread_cond_broadcast(&shared.visitor_can_enter); // Tell visitor to enter the museum
    }

    // Guide leaving
    shared.guide_leaving++;
    while (shared.visitors_inside != 0) {
      // Wait for all visitors to leave first
      pthread_cond_wait(&shared.guide_can_leave, &shared.mutex); 
    }
    shared.guide_leaving--;
    shared.guides_inside--;
    pthread_cond_signal(&shared.guide_can_enter); // Let guides outside know one can enter
    guide_leaves(id);
  }
  pthread_mutex_unlock(&shared.mutex);
}
