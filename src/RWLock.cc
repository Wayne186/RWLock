#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h> 
#include <sys/types.h> 
#include <sys/syscall.h> 
#include <assert.h>
#include <pthread.h>
#include <vector>
#include "RWLock.h"


RWLock::RWLock ()
{
	pthread_mutex_init(&m1, NULL);
	pthread_mutex_init(&m2, NULL);
	pthread_mutex_init(&m3, NULL);
	pthread_cond_init(&cond, NULL);
	std::vector<pthread_t> V;
	write_id = 0;
	counter = 1;
	nReaders = 0;
	sem = 1;
}

int RWLock::read_lock() 
{
	int i = 0;
	while ( i < V.size() && sem == 1) {
		if ( pthread_equal(V[i++], pthread_self()) )
			return 1;
	}
	
	if (pthread_equal(write_id, pthread_self())) 
		return 2;
	pthread_mutex_lock(&(this->m1));
	pthread_mutex_lock(&(this->m2));
	if (this->nReaders == 0 && sem != 0) {
		pthread_mutex_lock(&(this->m3));
		int j = this->counter;
		this->counter = j - sem;
		while (j < 1)
			pthread_cond_wait(&(this->cond), &(this->m3));
		sem = 1;
	}

	this->nReaders++;
	pthread_mutex_unlock(&(this->m3));
	pthread_mutex_unlock(&(this->m2));
	pthread_mutex_unlock(&(this->m1));
	this->V.push_back(pthread_self());
	return 0;
}

int RWLock::try_read_lock() 
{
	int i = 0;
	while (i < V.size() && sem == 1) {
		if ( pthread_equal(V[i++], pthread_self()) )
			return 1;
	}
	
	if (pthread_equal(write_id, pthread_self())) 
		return 2;

	if (write_id != 0 && sem != 0)
		return 5;

	pthread_mutex_lock(&(this->m1));
	pthread_mutex_lock(&(this->m2));
	if (this->nReaders == 0 && sem != 0) {
		pthread_mutex_lock(&(this->m3));
		int j = this->counter;
		this->counter = j - sem;
		while (j < 1)
			pthread_cond_wait(&(this->cond), &(this->m3));
	}
	this->nReaders++;
	pthread_mutex_unlock(&(this->m3));
	pthread_mutex_unlock(&(this->m2));
	pthread_mutex_unlock(&(this->m1));
	this->V.push_back(pthread_self());
	return 0;
}

int RWLock::read_unlock () 
{
	int i = 0;
	int f = 0;
	while (i < V.size() && sem == 1) {
		if ( pthread_equal(V[i++], pthread_self()) )
			f = sem;
	}
	if (f == 0)
		return 3;


	pthread_mutex_lock(&(this->m2));
	if (this->nReaders-- == 1 && sem != 0) {
		pthread_mutex_lock(&(this->m3));
		this->counter++;
		pthread_cond_signal(&(this->cond));
		pthread_mutex_unlock(&(this->m3));
	}
	pthread_mutex_unlock(&(this->m2));
	i = 0;
	while (i < V.size()) {
		if ( pthread_equal(V[i++], pthread_self()) ) {
			V.erase(V.begin()+i-1);
			return 0;
		}
	}
	return 0;
}

int RWLock::write_lock () 
{
	int i = 0;
	while (i < V.size() && sem == 1) {
		if ( pthread_equal(V[i++], pthread_self()) )
			return 1;
	}
	
	if (pthread_equal(write_id, pthread_self())) 
		return 2;
	pthread_mutex_lock(&(this->m1));
	write_id = pthread_self();
	pthread_mutex_lock(&(this->m3));
	sem = 1;
	this->counter--;
	while(this->counter < 0) 
		pthread_cond_wait(&(this->cond), &(this->m3));
	pthread_mutex_unlock(&(this->m3));
	return 0;
}

int RWLock::try_write_lock () 
{
	int i = 0;
	while (i < V.size() && sem == 1) {
		if ( pthread_equal(V[i++], pthread_self()) )
			return 1;
	}
	
	if (pthread_equal(write_id, pthread_self())) 
		return 2;

	if (nReaders != 0 && sem != 0)
		return 5;

	pthread_mutex_lock(&(this->m1));
	pthread_mutex_lock(&(this->m3));
	
	int j = this->counter;
	this->counter = j - sem;
	while (j < 1)
		pthread_cond_wait(&(this->cond), &(this->m3));
	pthread_mutex_unlock(&(this->m3));
	write_id = pthread_self();

	return 0;
}

int RWLock::write_unlock () 
{
	if ( (pthread_equal(write_id, pthread_self())) ) {
		pthread_mutex_lock(&(this->m3));
		this->counter++;
		write_id = 0;
		pthread_cond_signal(&(this->cond));
		pthread_mutex_unlock(&(this->m3));
		pthread_mutex_unlock(&(this->m1));
	} else {
		sem = 1;
		return 4;
	}
}

int RWLock::write_to_read () 
{
	if ( (pthread_equal(write_id, pthread_self())) ) {
		pthread_mutex_lock(&(this->m3));
		this->counter++;
		write_id = 0;
		pthread_cond_signal(&(this->cond));
		pthread_mutex_unlock(&(this->m3));
		pthread_mutex_unlock(&(this->m1));
	} else {
		sem = 1;
		return 4;
	}

	int i = 0;
	while (i < V.size() && sem == 1) {
		if ( pthread_equal(V[i++], pthread_self()) )
			return 1;
	}
	
	if (pthread_equal(write_id, pthread_self())) 
		return 2;
	
	pthread_mutex_lock(&(this->m1));
	pthread_mutex_lock(&(this->m2));
	if (this->nReaders == 0 && sem != 0) {
		pthread_mutex_lock(&(this->m3));
		int j = this->counter;
		this->counter = j - sem;
		while (j < 1)
			pthread_cond_wait(&(this->cond), &(this->m3));
	}

	this->nReaders++;
	pthread_mutex_unlock(&(this->m3));
	pthread_mutex_unlock(&(this->m2));
	pthread_mutex_unlock(&(this->m1));
	this->V.push_back(pthread_self());
	return 0;
}





