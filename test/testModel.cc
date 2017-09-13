/*
  @copyright Steve Keen 2015
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

#include "cairoItems.h"
#include "operation.h"
#include "group.h"
#include "minsky.h"
#include <ecolab_epilogue.h>

#include <UnitTest++/UnitTest++.h>
using namespace minsky;
using namespace std;

namespace
{

  struct TestFixture: public Minsky
  {
    GroupPtr group0;
    ItemPtr a,b,c;
    WirePtr ab,bc; 
    LocalMinsky lm;

    TestFixture(): lm(*this)
    {
      a=model->addItem(new Variable<VariableType::flow>("a"));
     b=model->addItem(new Variable<VariableType::flow>("b"));
     c=model->addItem(new Variable<VariableType::flow>("c"));
     // create 3 variables, wire them and add first two to a group,
     // leaving 3rd external
     a->moveTo(100,100);
     b->moveTo(200,100);
     c->moveTo(300,100);
     CHECK_EQUAL(2,a->ports.size());
     CHECK_EQUAL(2,b->ports.size());
     CHECK_EQUAL(2,c->ports.size());

     ab=model->addWire(new Wire(a->ports[0], b->ports[1]));
     bc=model->addWire(new Wire(b->ports[0], c->ports[1]));
     checkWiresConsistent();

     group0=model->addGroup(new Group);
     checkWiresConsistent();
     group0->addItem(a);
     checkWiresConsistent();
     group0->addItem(b);
     group0->splitBoundaryCrossingWires();
     
     CHECK_EQUAL(3,group0->items.size());
     CHECK_EQUAL(2,group0->wires.size());
     CHECK_EQUAL(1,model->items.size());
     CHECK_EQUAL(4,model->numItems());
     CHECK_EQUAL(3,model->numWires());
     checkWiresConsistent();

      // add a couple of time operators, to ensure the group has finite size
     model->addItem(OperationPtr(OperationType::time))->moveTo(100,75);
     model->addItem(OperationPtr(OperationType::time))->moveTo(200,125);
     canvas.select(50,50,250,150);
     save("TestGroupFixture.mky");
     CHECK(model->uniqueItems());
    }

    void checkWiresConsistent() {
      for (auto& i: model->items)
        for (auto& p: i->ports)
          for (auto& w: p->wires)
            {
              CHECK(w);
              CHECK(p->input() || p==w->from());
              CHECK(!p->input() || p==w->to());
            }
      for (auto& g: model->groups)
        for (auto& i: g->items)
          for (auto& p: i->ports)
            for (auto& w: p->wires)
              {
                assert(w);
                assert(p->input() || p==w->from());
                assert(!p->input() || p==w->to());
              }
    }
  };
}


SUITE(Group)
{
  TEST_FIXTURE(TestFixture, SelectGroup)
  {
    Group& g=*model->addGroup(new Group);
    g.addItem(a);
    g.addItem(b);
    CHECK_EQUAL(2, g.items.size());
    CHECK_EQUAL(1, g.wires.size());
    CHECK(&g==a->parent().get());
    CHECK(!a->visible());
    CHECK(&g==b->parent().get());
    CHECK(!b->visible()); 
    CHECK(c->parent()==model);
    CHECK(c->visible());
    CHECK(!model->findWire(*ab)->visible());
    CHECK(model->findWire(*bc)->visible());
    CHECK(find(g.wires.begin(), g.wires.end(), ab) != g.wires.end()); 
    CHECK(model->uniqueItems());
  }

  TEST_FIXTURE(TestFixture, addVariable)
  {
    group0->addItem(c);
    CHECK(model->uniqueItems());
    CHECK_EQUAL(4,group0->items.size());
    CHECK_EQUAL(2,model->items.size());
    CHECK_EQUAL(3,group0->wires.size());
    CHECK_EQUAL(0,model->wires.size());
    
    checkWiresConsistent();

    // now check removal
    group0->parent()->addItem(c);

    CHECK_EQUAL(3,group0->items.size());
    CHECK_EQUAL(2,group0->wires.size());
    CHECK_EQUAL(3,model->items.size());
    CHECK_EQUAL(1,model->wires.size());
  }

  // check that removing then adding an item leaves the group idempotent
  TEST_FIXTURE(TestFixture, removeAddItem)
  {
    CHECK_EQUAL(1,group0->createdIOvariables.size());
    CHECK_EQUAL(3,model->items.size());
    model->addItem(a);
    group0->splitBoundaryCrossingWires();
    save("x1.mky");
    CHECK_EQUAL(3,group0->items.size()); // extra io var created
    CHECK_EQUAL(4,model->items.size());
    CHECK_EQUAL(2,group0->createdIOvariables.size());
    CHECK_EQUAL(4,model->numWires());
    group0->addItem(a);
    group0->splitBoundaryCrossingWires();
    CHECK_EQUAL(3,group0->items.size());
    CHECK_EQUAL(3,model->items.size());
    CHECK_EQUAL(1,group0->createdIOvariables.size());
    CHECK_EQUAL(3,model->numWires());
    CHECK_EQUAL(3,group0->items.size());
  }

}

SUITE(Canvas)
{
  TEST_FIXTURE(TestFixture, getItemAt)
    {
      // zoom to display group0 to make a & b visible
      canvas.item=group0;
      canvas.zoomToDisplay();
      canvas.getItemAt(a->x()+2,a->y()+2);
      CHECK(a==canvas.item);
      canvas.getItemAt(b->x()-2,b->y()-2);
      CHECK(b==canvas.item);
      canvas.getItemAt(c->x()-2,c->y()+2);
      CHECK(c==canvas.item);
      canvas.getItemAt(group0->x()-2,group0->y()+2);
      CHECK(group0==canvas.item);
    }
  
  TEST_FIXTURE(TestFixture, getWireAt)
    {
      auto from=a->ports[0], to=b->ports[1];
      float x=0.5f*(from->x()+to->x())+1;
      float y=0.5f*(from->y()+to->y())+1;
      canvas.getWireAt(x,y);
      CHECK(canvas.wire==ab);
    }
  
  TEST_FIXTURE(Canvas,findVariableDefinition)
    {
      model=cminsky().model;
      VariablePtr var1(VariableType::flow,"foo");
      VariablePtr var2(VariableType::flow,"foo");
      OperationPtr op(OperationType::exp);      
      item=model->addItem(var1);
      model->addItem(var2);
      model->addItem(op);
      // initially, foo is undefined, so should return false
      CHECK(!findVariableDefinition());
      CHECK(item==var1);
      model->addWire(new Wire(op->ports[0],var2->ports[1]));
      CHECK(findVariableDefinition());
      CHECK(item==var2);
      model->removeItem(*var2);

      shared_ptr<IntOp> integ(new IntOp);
      integ->description("foo");
      model->addItem(integ);
      item=var1;
      CHECK(findVariableDefinition());
      CHECK(item==integ);
      model->removeItem(*integ);
      
      shared_ptr<GodleyIcon> godley(new GodleyIcon);
      model->addItem(godley);
      godley->table.resize(3,2);
      godley->table.cell(0,1)="foo";
      godley->table.cell(0,2)="bar";
      godley->update();
      item=var1;
      CHECK(findVariableDefinition());
      CHECK(item==godley);

      // on a non-variable, findVariableDefinition should returns false
      CHECK(!findVariableDefinition());

    }

    TEST_FIXTURE(TestFixture,moveItem)
    {
      cairo::Surface surf(cairo_recording_surface_create(CAIRO_CONTENT_COLOR,nullptr));
      c->draw(surf.cairo());// reposition ports
      CHECK(c->clickType(c->x(),c->y()) == ClickType::onItem);
      canvas.mouseDown(c->x(),c->y());
      canvas.mouseUp(400,500);
      CHECK_EQUAL(400,c->x());
      CHECK_EQUAL(500,c->y());
    }

    TEST_FIXTURE(TestFixture,onSlider)
    {
      auto cv=dynamic_cast<VariableBase*>(c.get());
      cv->value(1000);
      cv->sliderMin=0;
      cv->sliderMax=2000;
      // work out where slider is located
      RenderVariable rv(*cv);
      float xc=cv->x()+rv.handlePos(), yc=cv->y()-rv.height();
      CHECK_EQUAL(ClickType::onSlider, cv->clickType(xc,yc));
      canvas.mouseDown(xc,yc);
      xc+=5;
      canvas.mouseUp(xc,yc);
      // check handle and value changed
      CHECK_EQUAL(xc, c->x()+rv.handlePos());
      CHECK(cv->value()>1000);
      // check variable hasn't moved
      CHECK_EQUAL(300,cv->x());
      CHECK_EQUAL(100,cv->y());

      // now check that value is clamped to max/min
      canvas.mouseDown(xc,yc);
      canvas.mouseUp(xc+100,yc);
      CHECK_EQUAL(cv->sliderMax, cv->value());
      xc=cv->x()+rv.handlePos();
      canvas.mouseDown(xc,yc);
      canvas.mouseUp(xc-100,yc);
      CHECK_EQUAL(cv->sliderMin, cv->value());
    }

    TEST_FIXTURE(TestFixture,lasso)
    {
      canvas.selection.clear();
      canvas.mouseDown(250,50);
      canvas.mouseUp(350,150);
      CHECK_EQUAL(1,canvas.selection.items.size());
      CHECK(find(canvas.selection.items.begin(),canvas.selection.items.end(),c) !=canvas.selection.items.end());

      // now check when the first click is in the bounding box of an icon, but outside it
      OperationPtr op(OperationType::time);
      model->addItem(op);
      op->moveTo(500,500);
      float x=512, y=512;
      CHECK(op->contains(x,y));
      CHECK_EQUAL(ClickType::outside, op->clickType(x,y));
      canvas.selection.clear();
      canvas.mouseDown(x,y);
      canvas.mouseUp(x-5,y-5);
      CHECK_EQUAL(1,canvas.selection.items.size());
      CHECK(find(canvas.selection.items.begin(),canvas.selection.items.end(),op) !=canvas.selection.items.end());

      // test that groups can be selected
      CHECK(!group0->displayContents());
      float w=0.5*group0->width+5, h=0.5*group0->height+5;
      x=group0->x()-w; y=group0->y()-h;
      // nw -> se selection
      canvas.mouseDown(x,y);
      canvas.mouseUp(x+2*w,y+2*h);
      CHECK_EQUAL(0,canvas.selection.items.size());
      CHECK_EQUAL(1,canvas.selection.groups.size());
      CHECK(find(canvas.selection.groups.begin(),canvas.selection.groups.end(),group0) !=canvas.selection.groups.end());

      // ne -> sw selection
      canvas.selection.clear();
      canvas.mouseDown(x+2*w,y);
      canvas.mouseUp(x-2*w,y+2*h);
      CHECK_EQUAL(0,canvas.selection.items.size());
      CHECK_EQUAL(1,canvas.selection.groups.size());
      CHECK(find(canvas.selection.groups.begin(),canvas.selection.groups.end(),group0) !=canvas.selection.groups.end());
      
      // se -> nw selection
      canvas.selection.clear();
      canvas.mouseDown(x+2*w,y+2*h);
      canvas.mouseUp(x-2*w,y-2*h);
      CHECK_EQUAL(0,canvas.selection.items.size());
      CHECK_EQUAL(1,canvas.selection.groups.size());
      CHECK(find(canvas.selection.groups.begin(),canvas.selection.groups.end(),group0) !=canvas.selection.groups.end());
      
      // sw -> ne selection
      canvas.selection.clear();
      canvas.mouseDown(x,y+2*h);
      canvas.mouseUp(x+2*w,y-2*h);
      CHECK_EQUAL(0,canvas.selection.items.size());
      CHECK_EQUAL(1,canvas.selection.groups.size());
      CHECK(find(canvas.selection.groups.begin(),canvas.selection.groups.end(),group0) !=canvas.selection.groups.end());
    }

    TEST_FIXTURE(Canvas, wires)
      {
        model.reset(new Group);
        OperationPtr a(OperationType::exp);
        model->addItem(a);
        a->moveTo(100,100);
        OperationPtr b(OperationType::exp);
        model->addItem(b);
        b->moveTo(200,200);
        WirePtr w(new Wire(a->ports[0],b->ports[1]));
        model->addWire(w);
        auto coords=w->coords();
        CHECK_EQUAL(4, coords.size());

        // drag up, should insert a handle
        float x=0.5*(coords[0]+coords[2]), y=0.5*(coords[1]+coords[3]);
        mouseDown(x,y);
        x+=10;
        mouseUp(x,y);
        coords=w->coords();
        CHECK_EQUAL(6, coords.size());
        CHECK_EQUAL(x,coords[2]);
        CHECK_EQUAL(y,coords[3]);
      }

    TEST_FIXTURE(Canvas, godleyResize)
      {
        model.reset(new Group);
        addGodley();
        auto& godley=dynamic_cast<GodleyIcon&>(*itemFocus);
        CHECK(godley.width()!=200);
        CHECK(godley.height()!=200);
        mouseUp(200,200);
        getItemAt(200,200);
        lassoMode=LassoMode::itemResize;
        mouseUp(300,300);
        CHECK_EQUAL(200,godley.width());
        CHECK_EQUAL(200,godley.height());
      }

    TEST_FIXTURE(Canvas, moveIntoThenOutOfGroup)
      {
        model.reset(new Group);
        model->resetParent(model);
        OperationPtr a(OperationType::exp);
        model->addItem(a);
        a->moveTo(100,100);
        OperationPtr b(OperationType::exp);
        model->addItem(b);
        b->moveTo(200,200);
        model->addWire(*a,*b,1);

        auto g=model->addGroup(new Group);
        
        CHECK(b->parent()==model);
        CHECK_EQUAL(1,model->numWires());
        CHECK_EQUAL(2,model->numItems());
        CHECK_EQUAL(0,g->inVariables.size());

        // move b into group
        mouseDown(b->x(),b->y());
        mouseUp(g->x(),g->y());
        CHECK(b->parent()==g);
        CHECK_EQUAL(2,model->numWires());
        CHECK_EQUAL(3,model->numItems());
        CHECK_EQUAL(1,g->inVariables.size());

        // move b out of group
        item=g;
        zoomToDisplay();
        mouseDown(b->x(),b->y());
        mouseUp(200,200);
        CHECK(b->parent()==model);
        CHECK_EQUAL(1,model->numWires());
        CHECK_EQUAL(2,model->numItems());
        CHECK_EQUAL(0,g->inVariables.size());
      }
    
    TEST_FIXTURE(Canvas, mouseFocus)
      {
        model.reset(new Group);
        model->resetParent(model);
        OperationPtr a(OperationType::exp);
        model->addItem(a);
        a->moveTo(100,100);
        OperationPtr b(OperationType::exp);
        model->addItem(b);
        b->moveTo(200,200);
        auto w=model->addWire(*a,*b,1);

        CHECK(!a->mouseFocus);
        CHECK(!b->mouseFocus);
        CHECK(!w->mouseFocus);

        mouseMove(a->x(),a->y());
        CHECK(a->mouseFocus);
        CHECK(!b->mouseFocus);
        CHECK(!w->mouseFocus);

        mouseMove(b->x(),b->y());
        CHECK(!a->mouseFocus);
        CHECK(b->mouseFocus);
        CHECK(!w->mouseFocus);

        mouseMove(0.5*(a->x()+b->x()),0.5*(a->y()+b->y()));
        CHECK(!a->mouseFocus);
        CHECK(!b->mouseFocus);
        CHECK(w->mouseFocus);
        
      }

    TEST_FIXTURE(Canvas, removeItemFromItsGroup)
      {
        model.reset(new Group);
        model->resetParent(model);
        auto g0=model->addGroup(new Group);
        auto g1=g0->addGroup(new Group);
        auto a=g1->addItem(new Operation<OperationType::exp>);
        CHECK(a->parent()==g1);
        CHECK(a->visible()==g1->displayContents());
        item=a;
        removeItemFromItsGroup();
        CHECK(a->parent()==g0);
        CHECK(a->visible()==g0->displayContents());
        item=a;
        removeItemFromItsGroup();
        CHECK(a->parent()==model);
        CHECK(a->visible());
      }

    TEST_FIXTURE(TestFixture,selectAllVariables)
      {
        model->moveContents(*group0);
        canvas.item=a;
        canvas.copyItem();
        canvas.mouseUp(500,500);
        canvas.selectAllVariables();
        CHECK_EQUAL(2,canvas.selection.items.size());
        for (auto i: canvas.selection.items)
          {
            auto ii=dynamic_cast<VariableBase*>(i.get());
            CHECK(ii);
            if (ii) CHECK_EQUAL(dynamic_pointer_cast<VariableBase>(a)->valueId(), ii->valueId());
          }

        canvas.item=b;
        canvas.selectAllVariables();
        CHECK_EQUAL(1,canvas.selection.items.size());
        CHECK(canvas.selection.items[0]==b);

        canvas.item=group0;
        canvas.selectAllVariables();
        CHECK(canvas.selection.empty());

        // check integrals are selected too
        auto integ=new IntOp;
        canvas.item=model->addItem(integ);
        integ->description("foo");
        canvas.selectAllVariables();
        CHECK_EQUAL(1,canvas.selection.items.size());
        CHECK(canvas.selection.items[0]==integ->intVar);
      }
    
    TEST_FIXTURE(TestFixture,renameAllInstances)
      {
        model->moveContents(*group0);
        canvas.item=a;
        canvas.copyItem();
        canvas.mouseUp(500,500);
        canvas.renameAllInstances("foobar");
        unsigned count=0;
        for (auto i: model->items)
          if (auto v=dynamic_cast<VariableBase*>(i.get()))
            {
              CHECK(v->name()!="a");
              if (v->name()=="foobar")
                count++;
            }
        CHECK_EQUAL(2,count);

        // check no renaming should happen when item is not a variable 
        canvas.item=group0;
        canvas.renameAllInstances("foobar1");
        for (auto i: model->items)
          if (auto v=dynamic_cast<VariableBase*>(i.get()))
            CHECK(v->name()!="foobar1");
        
        // check integrals are renamed too
        auto integ=new IntOp;
        canvas.item=model->addItem(integ);
        integ->description("foo");
        canvas.renameAllInstances("bar");
        CHECK(integ->description()=="bar");
      }

    TEST_FIXTURE(TestFixture,ungroupItem)
      {
        unsigned originalNumItems=model->numItems();
        unsigned originalNumGroups=model->numGroups();
        canvas.item=group0;
        canvas.ungroupItem();
        CHECK_EQUAL(originalNumItems, model->numItems());
        CHECK_EQUAL(originalNumGroups-1, model->numGroups());
      }

    TEST_FIXTURE(TestFixture,copyItem)
      {
        model->addItem(a);
        unsigned originalNumItems=model->numItems();
        unsigned originalNumGroups=model->numGroups();
        canvas.item=a;
        canvas.copyItem();
        CHECK_EQUAL(originalNumItems+1, model->numItems());
        CHECK_EQUAL(dynamic_cast<VariableBase*>(a.get())->valueId(),
               dynamic_cast<VariableBase*>(model->items.back().get())->valueId());

        auto integ=new IntOp;
        canvas.item=model->addItem(integ);
        integ->description("foo");
        originalNumItems=model->numItems();
        canvas.copyItem();
        CHECK_EQUAL(originalNumItems+1, model->numItems());
        CHECK_EQUAL(integ->intVar->valueId(),
          dynamic_cast<VariableBase*>(model->items.back().get())->valueId());
      }
}

SUITE(Wire)
{
  TEST(handles)
    {
      Item item;
      item.ports.emplace_back(new Port(item));
      item.ports.back()->moveTo(0,0);
      item.ports.emplace_back(new Port(item));
      item.ports.back()->moveTo(10,10);
      Wire wire(item.ports[0],item.ports[1],{0,0,3,4,6,7,10,10});
      CHECK_EQUAL(0, wire.nearestHandle(3.1, 3.9));
      CHECK_EQUAL(1, wire.nearestHandle(6.2, 7.5));
      // should not have inserted anything yet
      CHECK_EQUAL(8, wire.coords().size());

      // check for midpoint insertion
      CHECK_EQUAL(0, wire.nearestHandle(1.6, 2));
      CHECK_EQUAL(10, wire.coords().size());
      CHECK_CLOSE(1.5, wire.coords()[2],0.01);
      CHECK_CLOSE(2, wire.coords()[3],0.01);
      
      CHECK_EQUAL(2, wire.nearestHandle(4.5, 5.5));
      CHECK_EQUAL(12, wire.coords().size());
      CHECK_CLOSE(4.5, wire.coords()[6],0.01);
      CHECK_CLOSE(5.5, wire.coords()[7],0.01);

      CHECK_EQUAL(4, wire.nearestHandle(10, 10));
      CHECK_EQUAL(14, wire.coords().size());
      CHECK_CLOSE(8, wire.coords()[10],0.01);
      CHECK_CLOSE(8.5, wire.coords()[11],0.01);

      wire.editHandle(0,2.2,3.3);
      CHECK_CLOSE(2.2, wire.coords()[2],0.01);
      CHECK_CLOSE(3.3, wire.coords()[3],0.01);
    }
}

