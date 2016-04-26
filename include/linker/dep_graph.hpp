/*
 * Rho - A sandbox for mathematics.
 * Copyright (C) 2015-2016 Jacob Zhitomirsky
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _RHO__LINKER__DEP_GRAPH__H_
#define _RHO__LINKER__DEP_GRAPH__H_

#include "linker/module.hpp"
#include <vector>
#include <unordered_map>
#include <deque>
#include <stdexcept>


namespace rho {
  
  /* 
   * Thrown to indicate that a dependency graph is not a DAG.
   */
  class dep_cycle_error: public std::runtime_error
  {
  public:
    dep_cycle_error (const std::string& str)
      : std::runtime_error (str)
      { }
  };
  
  
  class dependency_graph
  {
  private:
    enum node_status
    {
      UNMARKED,
      MARKED_TEMP,
      MARKED_PERM,
    };
    
    struct node
    {
      std::shared_ptr<module> m;
      node_status status;
    };
    
    struct edge
    {
      std::shared_ptr<node> from, to;
    };
    
  public:
    std::unordered_map<std::shared_ptr<module>, std::shared_ptr<node>> nodes;
    std::vector<edge> edges;
    
    std::deque<std::shared_ptr<node>> unmarked;
    
  private:
    std::shared_ptr<node> get_node (std::shared_ptr<module> m);
    
    void visit (std::shared_ptr<node> n,
                std::deque<std::shared_ptr<module>>& sorted);
    
  public:
    /* 
     * Marks :importer: as dependent on :importee:.
     */
    void add_dependency (std::shared_ptr<module> importer,
                         std::shared_ptr<module> importee);
    
    /* 
     * Returns an evaluation order obtained by performing a topological sort
     * on the dependency graph.
     * Throws 'dep_cycle_error' in case a cycle is encountered.
     */
    std::vector<std::shared_ptr<module>> get_evaluation_order ();
  };
}

#endif

