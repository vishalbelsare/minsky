/*
  @copyright Steve Keen 2018
  @author Russell Standish
  This file is part of Minsky.

  Minsky is free software: you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Minsky is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Minsky.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "godleyTable.h"
#include "selection.h"
#include "latexMarkup.h"
#include <pango.h>
#include <ecolab_epilogue.h>

using namespace std;
using namespace minsky;
using ecolab::Pango;

constexpr double GodleyTableWindow::leftTableOffset, GodleyTableWindow::topTableOffset;

void GodleyTableWindow::redraw(int, int, int width, int height)
{
  if (!godleyIcon) return;
  Pango pango(surface->cairo());
  pango.setMarkup(godleyIcon->table.cell(0,0));
  rowHeight=pango.height()+2;
  double tableHeight=(godleyIcon->table.rows()-scrollRowStart+1)*rowHeight;
  double x=leftTableOffset;
  double lastAssetBoundary=x;
  auto assetClass=GodleyAssetClass::noAssetClass;
  colLeftMargin.clear();
  
  for (unsigned col=0; col<godleyIcon->table.cols(); ++col)
    {
      // omit stock columns less than scrollColStart
      if (col>0 && col<scrollColStart) continue;
      colLeftMargin.push_back(x);
      double y=topTableOffset;
      double colWidth=0;
      for (unsigned row=0; row<godleyIcon->table.rows(); ++row)
        {
          if (row>0 && row<scrollRowStart) continue;
          if (row==0 && col==0)
            pango.setMarkup("Flows ↓ / Stock Vars →");
          else if (int(row)==selectedRow && int(col)==selectedCol)
            // the active cell renders as bare LaTeX code for editing
            pango.setMarkup(godleyIcon->table.cell(row,col));
          else
            pango.setMarkup(latexToPango(godleyIcon->table.cell(row,col)));
          colWidth=max(colWidth,pango.width());
          cairo_move_to(surface->cairo(),x+3,y);
          pango.show();
          y+=rowHeight;
        }
      y=topTableOffset;
      colWidth+=5;
      // vertical lines
      cairo_move_to(surface->cairo(),x,topTableOffset);
      cairo_rel_line_to(surface->cairo(),0,tableHeight);
      if (assetClass!=godleyIcon->table._assetClass(col))
        {
          if (assetClass!=GodleyAssetClass::noAssetClass)
            {
              pango.setMarkup(enumKey<GodleyAssetClass::AssetClass>(assetClass));
              cairo_move_to(surface->cairo(),0.5*(x+lastAssetBoundary-pango.width()),0);
              pango.show();
            }
          lastAssetBoundary=x;
          
          assetClass=godleyIcon->table._assetClass(col);
          cairo_move_to(surface->cairo(),x+3,topTableOffset);
          cairo_rel_line_to(surface->cairo(),0,tableHeight);
        }
      cairo_set_line_width(surface->cairo(),0.5);
      cairo_stroke(surface->cairo());

      x+=colWidth;
    }
  
  pango.setMarkup(enumKey<GodleyAssetClass::AssetClass>(assetClass));
  cairo_move_to(surface->cairo(),0.5*(x+lastAssetBoundary-pango.width()),0);
  pango.show();
  
  // final column vertical line
  colLeftMargin.push_back(x);
  cairo_move_to(surface->cairo(),x,topTableOffset);
  cairo_rel_line_to(surface->cairo(),0,tableHeight);
  cairo_move_to(surface->cairo(),x+3,topTableOffset);
  cairo_rel_line_to(surface->cairo(),0,tableHeight);
  cairo_set_line_width(surface->cairo(),0.5);
  cairo_stroke(surface->cairo());

  // now row sum column
  x+=3;
  double y=topTableOffset;
  cairo_move_to(surface->cairo(),x,y);
  pango.setMarkup("A-L-E");
  pango.show();
  double colWidth=pango.width();
  y+=rowHeight;
  
  for (unsigned row=1; row<godleyIcon->table.rows(); ++row)
    {
      if (row>0 && row<scrollRowStart) continue;
      pango.setMarkup(godleyIcon->table.rowSum(row));
      colWidth=max(colWidth,pango.width());
      cairo_move_to(surface->cairo(),x,y);
      pango.show();
      y+=rowHeight;
    }

  x+=colWidth;
  y=topTableOffset;
  for (unsigned row=0; row<=godleyIcon->table.rows(); ++row)
    {
      // horizontal lines
      if (row>0 && row<scrollRowStart) continue;
      cairo_move_to(surface->cairo(),leftTableOffset,y);
      cairo_line_to(surface->cairo(),x,y);
      cairo_set_line_width(surface->cairo(),0.5);
      cairo_stroke(surface->cairo());
      y+=rowHeight;
    }

  // final vertical line
  colLeftMargin.push_back(x);
  cairo_move_to(surface->cairo(),x,topTableOffset);
  cairo_rel_line_to(surface->cairo(),0,tableHeight);
  cairo_set_line_width(surface->cairo(),0.5);
  cairo_stroke(surface->cairo());
  
  // indicate selected cells
  cairo_save(surface->cairo());
  if (selectedRow==0 || (selectedRow>=scrollRowStart && selectedRow<godleyIcon->table.rows()))
    {
      size_t i=0, j=0;
      if (selectedRow>=scrollRowStart) j=selectedRow-scrollRowStart+1;
      double y=j*rowHeight+topTableOffset;
      if (selectedCol==0 || (selectedCol>=scrollColStart && selectedCol<godleyIcon->table.cols()))
        {
          if (selectedRow!=0 || selectedCol!=0) // can't select flows/stockVars label
            {
              if (selectedCol>=scrollColStart) i=selectedCol-scrollColStart+1;
              double x=colLeftMargin[i];
              cairo_set_source_rgba(surface->cairo(),1,1,1,1);
              cairo_rectangle(surface->cairo(),x,y,colLeftMargin[i+1]-x,rowHeight);
              cairo_fill(surface->cairo());
              pango.setMarkup(godleyIcon->table.cell(selectedRow,selectedCol));
              cairo_set_source_rgba(surface->cairo(),0,0,0,1);
              cairo_move_to(surface->cairo(),x,y);
              pango.show();
          }
        }
      else if (selectedCol<0) // whole row selected
        {
          cairo_rectangle(surface->cairo(),leftTableOffset,y,colLeftMargin.back()-leftTableOffset,rowHeight);
          cairo_set_source_rgba(surface->cairo(),1,1,1,0.5);
          cairo_fill(surface->cairo());
        }
    }
  else if (selectedRow<0 && selectedCol>=scrollColStart && selectedCol<godleyIcon->table.cols()) // whole column selected
    {
      double x=colLeftMargin[selectedCol-scrollColStart+1];
      double width=colLeftMargin[selectedCol-scrollColStart+2]-x;
      cairo_rectangle(surface->cairo(),x,topTableOffset,width,tableHeight);
      cairo_set_source_rgba(surface->cairo(),1,1,1,0.5);
      cairo_fill(surface->cairo());
    }
  cairo_restore(surface->cairo());
}

void GodleyTableWindow::mouseDown(float x, float y)
{
  auto p=std::upper_bound(colLeftMargin.begin(), colLeftMargin.end(), x);
  if (p>colLeftMargin.begin())
    selectedCol=p-colLeftMargin.begin()-1;
  if (selectedCol>0) selectedCol+=scrollColStart-1;

  selectedRow=(y-topTableOffset)/rowHeight;
  requestRedraw();
}

void GodleyTableWindow::mouseUp(float x, float y)
{
}

void GodleyTableWindow::mouseMove(float x, float y)
{
}