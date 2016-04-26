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

#include "linker/dep_graph.hpp"


namespace rho {
  
  std::shared_ptr<dependency_graph::node>
  dependency_graph::get_node (std::shared_ptr<module> m)
  {
    auto itr = this->nodes.find (m);
    if (itr != this->nodes.end ())
      return itr->second;
    
    auto n = this->nodes[m] = std::shared_ptr<node> (new node ());
    n->m = m;
    n->status = UNMARKED;
    this->unmarked.push_back (n);
    return n;
  }
  
  
  
  /* 
   * Marks :importer: as dependent on :importee:.
   */
  void
  dependency_graph::add_dependency (std::shared_ptr<module> importer,
                                    std::shared_ptr<module> importee)
  {
    auto from = this->get_node (importee);
    auto to = this->get_node (importer);
    
    this->edges.push_back ({ .from = from, .to = to });
  }
  
  
  
  void
  dependency_graph::visit (std::shared_ptr<node> n,
                           std::deque<std::shared_ptr<module>>& sorted)
  {
    if (n->status == MARKED_TEMP)
      throw dep_cycle_error ("encountered cycle in dependency graph");
    else if (n->status == UNMARKED)
      {
        n->status = MARKED_TEMP;
        for (auto e : this->edges)
          if (e.from == n)
            this->visit (e.to, sorted);
        
        n->status = MARKED_PERM;
        sorted.push_front (n->m);
      }
  }
  
  /* 
   * Returns an evaluation order obtained by performing a topological sort
   * on the dependency graph.
   * Throws 'dep_cycle_error' in case a cycle is encountered.
   */
  std::vector<std::shared_ptr<module>>
  dependency_graph::get_evaluation_order ()
  {
    std::deque<std::shared_ptr<module>> sorted;
    while (!this->unmarked.empty ())
      {
        auto n = this->unmarked.back ();
        this->unmarked.pop_back ();
        
        this->visit (n, sorted);
      }
    
    return std::vector<std::shared_ptr<module>> (sorted.begin (), sorted.end ());
  }
}

