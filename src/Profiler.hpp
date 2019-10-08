/**
 * @file Profiler.hpp
 * @author Thomas Gillis and Denis-Gabriel Caprace
 * @copyright Copyright © UCLouvain 2019
 * 
 * FLUPS is a Fourier-based Library of Unbounded Poisson Solvers.
 * 
 * Copyright (C) <2019> <Universite catholique de Louvain (UCLouvain), Belgique>
 * 
 * List of the contributors to the development of FLUPS, Description and complete License: see LICENSE file.
 * 
 * This program (FLUPS) is free software: 
 * you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program (see COPYING file).  If not, 
 * see <http://www.gnu.org/licenses/>.
 * 
 */

#ifndef PROFILER_HPP
#define PROFILER_HPP
#include <iostream>
#include <map>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <list>
#include "defines.hpp"
#include "mpi.h"

using namespace std;

class TimerAgent {
   protected:
    bool   _isroot  = true;
    int    _count   = 0;
    double _timeAcc = 0.0;
    double _t0      = 0.0;
    double _t1      = 0.0;
    size_t _memsize = 0;

    double _timeMax = 0.0;
    double _timeMin = 0.0;

    string _name = "noname";

    TimerAgent*       _daddy = NULL;
    map<string,TimerAgent*> _children;

   public:
    TimerAgent(string name);

    void start();
    void stop();
    void reset();
    void addMem(size_t mem);
    void disp(FILE* file, const int level, const double totalTime);

    int    count() const { return _count; };
    bool   isroot() const { return _isroot; };
    string name() const { return _name; };
    double timeAcc() const;
    double timeMin() const;
    double timeMax() const;

    void addChild(TimerAgent* child);
    void setDaddy(TimerAgent* daddy);
    void writeParentality(FILE* file, const int level);
};

class Profiler {
   protected:
    map<string, TimerAgent*> _timeMap;

    string _name;
    void _createSingle(string name);

   public:
    Profiler();
    Profiler(string myname);
    ~Profiler();

    void create(string name);
    void create(string child, string daddy);

    void start(string name);
    void stop(string name);
    void addMem(string name,size_t mem);

    double get_timeAcc(const std::string ref);

    void disp();
    void disp(const std::string ref);
};

#endif
