//  
//  For more information, please see: http://software.sci.utah.edu
//  
//  The MIT License
//  
//  Copyright (c) 2006 Scientific Computing and Imaging Institute,
//  University of Utah.
//  
//  License for the specific language governing rights and limitations under
//  Permission is hereby granted, free of charge, to any person obtaining a
//  copy of this software and associated documentation files (the "Software"),
//  to deal in the Software without restriction, including without limitation
//  the rights to use, copy, modify, merge, publish, distribute, sublicense,
//  and/or sell copies of the Software, and to permit persons to whom the
//  Software is furnished to do so, subject to the following conditions:
//  
//  The above copyright notice and this permission notice shall be included
//  in all copies or substantial portions of the Software.
//  
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
//  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
//  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//  DEALINGS IN THE SOFTWARE.
//  
//  
//    File   : Text.cc
//    Author : McKay Davis
//    Date   : Tue Jun 27 13:00:29 2006
#include <Core/Skinner/Text.h>
#include <Core/Skinner/Variables.h>
#include <Core/Geom/FontManager.h>
#include <Core/Containers/StringUtil.h>

namespace SCIRun {
  namespace Skinner {
    Text::Text(Variables *vars) :
      Drawable(vars),
      fgcolor_(vars,"fgcolor",Color(0.0, 1.0, 0.0, 1.)),
      bgcolor_(vars,"bgcolor"),
      flags_(0),
      renderer_(0),
      offsetx_(),
      offsety_(),
      cursor_position_(0),
      text_(vars, "text"),
      cursor_(vars, "cursor", false)
    {
      REGISTER_CATCHER_TARGET(Text::redraw);

      if (!bgcolor_.exists()) {
        cerr << "bgcolor not exists: " << get_id() << std::endl;
        Var<Color> test(vars, "breakpoint");
        bgcolor_ = Color(1.0, 0.0, 0.0, 1.0);
      }
      
      Var<double>size(vars, "size", 20.0);
      Var<string>font(vars, "font", "scirun.ttf");
      renderer_ = FontManager::get_renderer(size(), font());
      
      offsetx_ = Var<int>(vars, "offsetx");
      offsetx_ |= Var<int>(vars, "offset");
      offsetx_ |= 0;

      offsety_ = Var<int>(vars, "offsety");
      offsety_ |= Var<int>(vars, "offset");
      offsety_ |= 0;

      Var<string> anchorstr(vars, "anchor", "SW");
      anchorstr = string_toupper(anchorstr());

      flags_ = TextRenderer::SW;
      if      (anchorstr() ==  "N") { flags_ = TextRenderer::N;  }
      else if (anchorstr() ==  "E") { flags_ = TextRenderer::E;  }
      else if (anchorstr() ==  "S") { flags_ = TextRenderer::S;  }
      else if (anchorstr() ==  "W") { flags_ = TextRenderer::W;  }
      else if (anchorstr() == "NE") { flags_ = TextRenderer::NE; }
      else if (anchorstr() == "SE") { flags_ = TextRenderer::SE; }
      else if (anchorstr() == "SW") { flags_ = TextRenderer::SW; }
      else if (anchorstr() == "NW") { flags_ = TextRenderer::NW; }
      else if (anchorstr() ==  "C") { flags_ = TextRenderer::C;  }
      else { cerr << vars->get_id() << " anchor invalid: " 
                  << anchorstr() << "\n"; }
      flags_ |= Var<bool>(vars,"vertical",0)() ? TextRenderer::VERTICAL : 0;
      flags_ |= Var<bool>(vars,"shadow",0)()   ? TextRenderer::SHADOW   : 0;
      flags_ |= Var<bool>(vars,"extruded",0)() ? TextRenderer::EXTRUDED : 0;
      flags_ |= Var<bool>(vars,"reverse",0)()  ? TextRenderer::REVERSE  : 0;
      
      //      REGISTER_CATCHER_TARGET(Text::redraw);
    }

    Text::~Text() {}
#if 0
    BaseTool::propagation_state_e
    Text::process_event(event_handle_t event)
    {
      WindowEvent *window = dynamic_cast<WindowEvent *>(event.get_rep());
      if (window && window->get_window_state() == WindowEvent::REDRAW_E) {
        redraw(0);
      }
      return CONTINUE_E;
    }
#endif

    BaseTool::propagation_state_e
    Text::redraw(event_handle_t)
    {
      if (!renderer_) return CONTINUE_E;
      const RectRegion &region = get_region();
      if (!text_.exists()) text_ = "ABC123";
      if (region.height() < 1 || renderer_->height(text_()) > region.height()) {
        return STOP_E;
      }
      
      int newflags = flags_;
      if (cursor_()) {
        newflags |= TextRenderer::CURSOR;
      } else {
        newflags &= ~TextRenderer::CURSOR;
      }

      flags_ = newflags;

      renderer_->set_shadow_offset(offsetx_(), offsety_());
      renderer_->set_color(fgcolor_().r, fgcolor_().g, fgcolor_().b, fgcolor_().a);
      renderer_->set_shadow_color(bgcolor_().r, bgcolor_().g, bgcolor_().b, bgcolor_().a);
      
      float mx = (region.x2() + region.x1())/2.0;
      float my = (region.y2() + region.y1())/2.0;
      
      float x = mx;
      float y = my;
      
      switch (flags_ & TextRenderer::ANCHOR_MASK) {
      case TextRenderer::N:  x = mx; y = region.y2(); break;
      case TextRenderer::S:  x = mx; y = region.y1(); break;
        
      case TextRenderer::E:  x = region.x2(); y = my; break;
      case TextRenderer::W:  x = region.x1(); y = my; break;
        
      case TextRenderer::NE: x = region.x2(); y = region.y2(); break;
      case TextRenderer::SE: x = region.x2(); y = region.y1(); break;
      case TextRenderer::SW: x = region.x1(); y = region.y1(); break;
      case TextRenderer::NW: x = region.x1(); y = region.y2(); break;
        
      case TextRenderer::C:  x = mx; y = my; break;
      }
      renderer_->render(text_(), x, y, flags_);
      
      return CONTINUE_E;
    }


  }
}
