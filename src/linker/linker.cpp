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

#include "linker/linker.hpp"
#include "linker/dep_graph.hpp"
#include <algorithm>

#include <iostream> // DEBUG


namespace rho {
  
  linker::linker ()
  {
    this->mod_idx = 1;
  }
  
  
  
  void
  linker::set_next_mod_idx (int next_mod_idx)
  {
    this->mod_idx = next_mod_idx;
  }
  
  
  
  /* 
   * Inserts the specified module into the list of modules to be linked
   * together.
   */
  void
  linker::add_module (std::shared_ptr<module> m)
  {
    this->mods.push_back (m);
  }
  
  
  
  bool
  linker::is_known_module (const std::string& mident)
  {
    auto itr = this->known_mods.find (mident);
    return itr != this->known_mods.end ();
  }
  
  void
  linker::add_known_module (const std::string& mident, int midx)
  {
    this->known_mods[mident] = midx;
  }
  
  
  
  /* 
   * Links all modules inserted so far.
   */
  std::shared_ptr<program>
  linker::link ()
  {
    this->init ();
    
    for (auto m : this->smods)
      this->link_in (m);
    this->fix_relocations ();
    
    this->cgen.emit_exit ();
    
    return std::shared_ptr<program> (
      new program (this->cgen.data (), this->cgen.size ()));
  }
  
  
  
  void
  linker::init ()
  {
    for (auto m : this->mods)
      {
        auto itr = this->mod_map.find (m->get_name ());
        if (itr != this->mod_map.end ())
          throw link_error ("same module is linked more than once");
        this->mod_map[m->get_name ()] = m;
        
        if (!this->is_known_module (m->get_name ()))
          {
            auto& inf = this->infos[m->get_name ()];
            inf.mod = m;
            inf.code_off = 0;
            inf.idx = 0;
          }
      }
    
    // determine proper evaluation order
    dependency_graph dg;
    for (auto m : this->mods)
      for (auto& imp : m->get_imports ())
        dg.add_dependency (m, this->mod_map[imp]);
    this->smods = dg.get_evaluation_order ();
    if (this->smods.empty ())
      this->smods = this->mods;
  }
  
  
  
  void
  linker::link_in (std::shared_ptr<module> m)
  {
    auto itr = this->known_mods.find (m->get_name ());
    if (itr != this->known_mods.end ())
      {
        return;
      }
    
    auto& inf = this->infos[m->get_name ()];
    inf.idx = (m->get_name () == "#this#" ? 0 : this->mod_idx++);
    
    int moff = this->cgen.get_label_pos (this->cgen.make_and_mark_label ());    
    inf.code_off = moff;
    
    this->cgen.put_bytes (m->get_code (), m->get_code_size ());
    if (m != this->smods.back ())
      this->cgen.emit_pop ();
  }
  
  
  
  void
  linker::fix_relocations ()
  {
    for (auto m : this->mods)
      {
        if (this->is_known_module (m->get_name ()))
          continue;
        
        auto& inf = this->infos[m->get_name ()];
        for (auto rel : m->get_relocs ())
          {
            this->cgen.seek (inf.code_off + rel.pos);
            switch (rel.type)
              {
              // fix global page index
              case REL_GP:
                this->cgen.put_short (inf.idx);
                break;
              
              // fix global variable retrieval from another module
              case REL_GV:
                {
                  auto itr = this->known_mods.find (rel.mname);
                  if (itr == this->known_mods.end ())
                    {
                      auto inf = this->infos[rel.mname];
                      this->cgen.put_short (inf.idx);
                    }
                  else
                    {
                      this->cgen.put_short (itr->second);
                    }
                }
                break;
              }
          }
      }
    
    this->cgen.seek_to_end ();
  }
}

