#if 1

#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Classlib/Assert.h>
#include <Classlib/Persistent.h>
#include <Classlib/String.h>
#include <Math/MinMax.h>
#include <Math/MiscMath.h>
#include <iostream.h>
#include <stdio.h>

Point Interpolate(const Point& p1, const Point& p2, double w)
{
    return Point(
	Interpolate(p1._x, p2._x, w),
	Interpolate(p1._y, p2._y, w),
	Interpolate(p1._z, p2._z, w));
}

clString Point::string() const
{
#if 0
    return clString("[")
	+to_string(_x)+clString(", ")
	    +to_string(_y)+clString(", ")
		+to_string(_z)+clString("]");
#endif
    char buf[100];
    sprintf(buf, "[%g, %g, %g]", _x, _y, _z);
    return clString(buf);
}

int Point::operator==(const Point& p) const
{
    return p._x == _x && p._y == _y && p._z == _z;
}

int Point::operator!=(const Point& p) const
{
    return p._x != _x || p._y != _y || p._z != _z;
}

Point::Point(double x, double y, double z, double w)
{
    if(w==0){
	cerr << "degenerate point!" << endl;
	_x=_y=_z=0;
    } else {
	_x=x/w;
	_y=y/w;
	_z=z/w;
    }
#if SCI_ASSERTION_LEVEL >= 4
    uninit=0;
#endif
}

Point AffineCombination(const Point& p1, double w1,
			const Point& p2, double w2)
{
    return Point(p1._x*w1+p2._x*w2,
		 p1._y*w1+p2._y*w2,
		 p1._z*w1+p2._z*w2);
}

Point AffineCombination(const Point& p1, double w1,
			const Point& p2, double w2,
			const Point& p3, double w3)
{
    return Point(p1._x*w1+p2._x*w2+p3._x*w3,
		 p1._y*w1+p2._y*w2+p3._y*w3,
		 p1._z*w1+p2._z*w2+p3._z*w3);
}

Point AffineCombination(const Point& p1, double w1,
			const Point& p2, double w2,
			const Point& p3, double w3,
			const Point& p4, double w4)
{
    return Point(p1._x*w1+p2._x*w2+p3._x*w3+p4._x*w4,
		 p1._y*w1+p2._y*w2+p3._y*w3+p4._y*w4,
		 p1._z*w1+p2._z*w2+p3._z*w3+p4._z*w4);
}

ostream& operator<<( ostream& os, const Point& p )
{
   os << p.string();
   return os;
}

#if 1 
istream& operator>>( istream& is, Point& v)
{
    double x, y, z;
    char st;
  is >> st >> x >> st >> y >> st >> z >> st;
  v=Point(x,y,z);
  return is;
}
#endif

void Pio(Piostream& stream, Point& p)
{
    stream.begin_cheap_delim();
    Pio(stream, p._x);
    Pio(stream, p._y);
    Pio(stream, p._z);
    stream.end_cheap_delim();
}


int
Point::Overlap( double a, double b, double e )
{
  double hi, lo, h, l;
  
  hi = a + e;
  lo = a - e;
  h  = b + e;
  l  = b - e;

  if ( ( hi > l ) && ( lo < h ) )
    return 1;
  else
    return 0;
}
  
int
Point::InInterval( Point a, double epsilon )
{
  if ( Overlap( _x, a.x(), epsilon ) &&
      Overlap( _y, a.y(), epsilon )  &&
      Overlap( _z, a.z(), epsilon ) )
    return 1;
  else
    return 0;
}

void Point::test_rigorous(RigorousTest* __test)
{	


    //Basic Point Tests

    Point P(0,0,0);
    Point Q(0,0,0);

    for(int x=0;x<=100;++x){
	for(int y=0;y<=100;++y){
	    for(int z=0;z<=100;++z){
		P.x(x);
		P.y(y);
		P.z(z);
		
		TEST(P.x()==x);
		TEST(P.y()==y);
		TEST(P.z()==z);
		
		Q=P;	
		TEST(Q.x()==x);
		TEST(Q.y()==y);
		TEST(Q.z()==z);
		TEST(P==Q);
		Q.x(x+1);
		Q.y(y+1);
		Q.z(z+1);
		TEST(P!=Q);
	    }
	}
    }
    
    //with doubles

    for(x=0;x<=100;++x){
	for(int y=0;y<=100;++y){
	    for(int z=0;z<=100;++z){
		P.x((double)x-.00007);
		P.y((double)y-.00007);
		P.z((double)z-.00007);
		
		TEST(P.x()==(double)x-.00007);
		TEST(P.y()==(double)y-.00007);
		TEST(P.z()==(double)z-.00007);
		
		Q=P;	
		TEST(Q.x()==P.x());
		TEST(Q.y()==P.y());
		TEST(Q.z()==P.z());
		TEST(P==Q);
		Q.x(x+1);
		Q.y(y+1);
		Q.z(z+1);
		TEST(P!=Q);
	    }
	}
    }
   
    //Basic Vector Tests

    Vector V(0,0,0);
    Vector V2(0,0,0);

    for(x=0;x<=100;++x){
	for(int y=0;y<=100;++y){
	    for(int z=0;z<=100;++z){
		V.x(x);
		V.y(y);
		V.z(z);

		TEST(V.x()==x);
		TEST(V.y()==y);
		TEST(V.z()==z);
    
		V2=V;
		TEST(V2.x()==x);
		TEST(V2.y()==y);
		TEST(V2.z()==z);
		TEST(V==V2);
		
		V2.x(-1);
		TEST(!(V==V2));
		V2.x(x);
		
		TEST(V==V2);
		V2.y(-1);
		TEST(!(V==V2));
		V2.y(y);

             	TEST(V==V2);
		V2.z(z+1);
		TEST(!(V==V2));
	    }
	}
    }
    
    //with doubles

    for(x=0;x<=100;++x){
	for(int y=0;y<=100;++y){
	    for(int z=0;z<=100;++z){
		V.x((double)x-.00007);
		V.y((double)y-.00007);
		V.z((double)z-.00007);

		TEST(V.x()==(double)x-.00007);
		TEST(V.y()==(double)y-.00007);
		TEST(V.z()==(double)z-.00007);
		V2=V;
		TEST(V.x()==V2.x());
		TEST(V.y()==V2.y());
		TEST(V.z()==V2.z());
		TEST(V==V2);
		
		V.x(x);
	        TEST(!(V==V2));
		
		V.x((double)x-.00007);
		
	       
		TEST(V==V2);
	    }	
	}
    }

    //Operator tests

    Point p1(0,0,0);
    Point p2(0,0,0);
    Point p3(0,0,0);

    Vector v1(0,0,0);
    Vector v2(0,0,0);
    Vector v3(0,0,0);
    int X,Y,Z;
    double Xd,Yd,Zd;
    const double cnst = 1234.5678;      
    Point m_A(1,2,3);
    Point m_A_prime(1,2,3);
    for (x=0;x<10;++x){
	for (int y=0;y<10;++y){
	    for (int z=0;z<10;++z){
		p1.x(x);
		p1.y(y);
		p1.z(z);
		TEST((p1.x()==x)&&(p1.y()==y)&&(p1.z()==z));
		
		p2.x(z);
		p2.y(x);
		p2.z(y);
		TEST((p2.x()==z)&&(p2.y()==x)&&(p2.z()==y));

		v1.x(x);
		v1.y(y);
		v1.z(z);
		TEST((v1.x()==x)&&(v1.y()==y)&&(v1.z()==z));
		
		v2.x(z);
		v2.y(x);
		v2.z(y);
		TEST((v2.x()==z)&&(v2.y()==x)&&(v2.z()==y));
		
		//Point-Point=Vector Tests
		
		v3=p2-p1;
		X=p2.x()-p1.x();
		Y=p2.y()-p1.y();
		Z=p2.z()-p1.z();

		TEST(v3.x()==X);
		TEST(v3.y()==Y);
		TEST(v3.z()==Z);
		
		//Point+Vector=Point Tests
		
		p2=p1+v1;
		X=p1.x()+v1.x();
		Y=p1.y()+v1.y();
		Z=p1.z()+v1.z();

		TEST(p2.x()==X);
		TEST(p2.y()==Y);
		TEST(p2.z()==Z);
		
		//Point+=Vector Tests
		p1.x(x);
		p1.y(y);
		p1.z(z);
		TEST((p1.x()==x)&&(p1.y()==y)&&(p1.z()==z));
		v1.x(z);
		v1.y(x);
		v1.z(y);
		TEST((v1.x()==z)&&(v1.y()==x)&&(v1.z()==y));
		X = p1.x()+v1.x();
		Y = p1.y()+v1.y();
		Z = p1.z()+v1.z();

		p1+=v1;
		TEST(p1.x()==X);
		TEST(p1.y()==Y);
		TEST(p1.z()==Z);
	      
		
		//Point-Vector=Point Tests
		
		p1.x(x);
		p1.y(y);
		p1.z(z);
		TEST((p1.x()==x)&&(p1.y()==y)&&(p1.z()==z));
		v1.x(z);
		v1.y(x);
		v1.z(y);
		TEST((v1.x()==z)&&(v1.y()==x)&&(v1.z()==y));
		
		X=p1.x()-v1.x();
		Y=p1.y()-v1.y();
		Z=p1.z()-v1.z();
	  
		
		p3=p1-v1;
		TEST(p3.x()==X);
		TEST(p3.y()==Y);
		TEST(p3.z()==Z);

		//Point-=Vector Tests
		X = p1.x()-v1.x();
		Y = p1.y()-v1.y();
		Z = p1.z()-v1.z();
		
		p1-=v1;
		
		TEST(p1.x()==X);
		TEST(p1.y()==Y);
		TEST(p1.z()==Z);
		
		//Constant*Point=Point Tests

		m_A_prime=m_A*cnst;
		
		Xd = m_A.x()*cnst;
		Yd = m_A.y()*cnst;
		Zd = m_A.z()*cnst;

		TEST (m_A_prime.x()==Xd);
		TEST (m_A_prime.y()==Yd);
		TEST (m_A_prime.z()==Zd);
		
		//Point*=Point Tests
		
		m_A_prime.x(double(x));
		m_A_prime.y(double(y));
		m_A_prime.z(double(z));

		TEST(m_A_prime.x()==x&&m_A_prime.y()==y&&m_A_prime.z()==z);
		
		m_A_prime=m_A_prime*cnst;

		Xd=m_A_prime.x()*cnst;
		Yd=m_A_prime.y()*cnst;
		Zd=m_A_prime.z()*cnst;

		m_A_prime*=cnst;

		TEST(m_A_prime.x()==Xd);
		TEST(m_A_prime.y()==Yd);
		TEST(m_A_prime.z()==Zd);



		//Point/Constant=Point Tests

		m_A_prime=m_A/cnst;
		
		Xd = m_A.x()/cnst;
		Yd = m_A.y()/cnst;
		Zd = m_A.z()/cnst;

		TEST (m_A_prime.x()==Xd);
		TEST (m_A_prime.y()==Yd);
		TEST (m_A_prime.z()==Zd);
		
		//Point/=Constant Tests

		Xd = m_A_prime.x()/cnst;
		Yd = m_A_prime.y()/cnst;
		Zd = m_A_prime.z()/cnst;

		m_A_prime/=cnst;
		
		TEST(m_A_prime.x()==Xd);
		TEST(m_A_prime.y()==Yd);
		TEST(m_A_prime.z()==Zd);
	
		//Point operator- const Tests
		
		Point pt(x,y,z);
		X = pt.x()-(2*pt.x());
		Y = pt.y()-(2*pt.y());
		Z = pt.z()-(2*pt.z());

		pt = -pt;
		
		TEST(pt.x()==X);
		TEST(pt.y()==Y);
		TEST(pt.z()==Z);
		
	        //Dot Multiplication Tests
		
		Point a1(x,y,z);
		Point a2(z,x,y);
		
		Vector b1(x,y,z);
		Vector b2(z,x,y);

		TEST((a1.x()==x)&&(a1.y()==y)&&(a1.z()==z));
		TEST((a2.x()==z)&&(a2.y()==x)&&(a2.z()==y));

		TEST((b1.x()==x)&&(b1.y()==y)&&(b1.z()==z));
		TEST((b2.x()==z)&&(b2.y()==x)&&(b2.z()==y));
		

		//Dot(Point,Point) Test
		TEST((Dot(a1,a2))==((a1.x()*a2.x())+(a1.y()*a2.y())+(a1.z()*a2.z())));

		//Dot(Point,Vector) Test
		TEST((Dot(a1,b2))==((a1.x()*b2.x())+(a1.y()*b2.y())+(a1.z()*b2.z())));

		//Dot(Vector,Point) Test
		TEST((Dot(b1,a2))==((b1.x()*a2.x())+(b1.y()*a2.y())+(b1.z()*a2.z())));

		//Dot(Vector,Vector) Test
		TEST((Dot(b1,b2))==((b1.x()*b2.x())+(b1.y()*b2.y())+(b1.z()*b2.z())));
		
		//Vector() Tests
		Point p(x,y,z);		
		TEST((p+p.vector())==(p*2));	//Make sure p.vector() is a vector
		
		
		//Max Tests
		Point pa(x,y,z); //These Points are also used for the AffineCombination() Tests
		Point pb(z,x,y);
		Point pc(y,z,x);
		Point pd((z-10),(x+15),(y-7));
		
		Point max=Max(pa,pb);

		if(pa.x()>=pb.x())
		  TEST(max.x()==pa.x());
		if(pa.x()<pb.x())
		  TEST(max.x()==pb.x());

		if(pa.y()>=pb.y())
		  TEST(max.y()==pa.y());
		if(pa.y()<pb.y())
		  TEST(max.y()==pb.y());

		if(pa.z()>=pb.z())
		  TEST(max.z()==pa.z());
		if(pa.z()<pb.z())
		  TEST(max.z()==pb.z());

		//Min Tests
		Point min=Min(pa,pb);

		if(pa.x()<=pb.x())
		  TEST(min.x()==pa.x());
		if(pa.x()>pb.x())
		  TEST(min.x()==pb.x());

		if(pa.y()<=pb.y())
		  TEST(min.y()==pa.y());
		if(pa.y()>pb.y())
		  TEST(min.y()==pb.y());

		if(pa.z()<=pb.z())
		  TEST(min.z()==pa.z());
		if(pa.z()>pb.z())
		  TEST(min.z()==pb.z());
				  
		


		//AffineCombination Tests

		double c=0;
		double d=0;
		double e=0;
		double f=0;
		Point a;
		
		for(int i=1;i<=10;++i){
		  TEST(pa.x()==x);
		  
		  c = .1*double(i);
		  d = 1-c;

		  a = AffineCombination(pa,c,pb,d);
		  TEST(a.x()==((c*pa.x())+(d*pb.x())));
		  TEST(a.y()==((c*pa.y())+(d*pb.y())));
		  TEST(a.z()==((c*pa.z())+(d*pb.z())));
		  
		}
		    
		for(i=0;i<=10;++i){
		  c=.1*double(i);
		  for(int i2=0;i2<=(10-i);++i2){
		    d = .1*double(i2);
		    e = (1-(c+d));
		    
		    TEST(c+d+e==1);

		    a = AffineCombination(pa,c,pb,d,pc,e);
		    TEST((a.x())==((c*pa.x())+(d*pb.x())+(e*pc.x())));
		    TEST((a.y())==((c*pa.y())+(d*pb.y())+(e*pc.y())));
		    TEST((a.z())==((c*pa.z())+(d*pb.z())+(e*pc.z())));   
		  }
		}	


		for(i=0;i<=10;++i){
		  c=.1*double(i);
		  for(int i2=0;i2<=(10-i);++i2){
		    d=.1*double(i2);
		    for(int i3=0;i3<=(10-(i+i2));++i3){
		      e=.1*double(i3);
		      f=1-(c+d+e);
		      TEST(c+d+e+f==1);
		      a = AffineCombination(pa,c,pb,d,pc,e,pd,f);
		      TEST((a.x())==((c*pa.x())+(d*pb.x())+(e*pc.x())+(f*pd.x())));
		     
		      
		    }
		  }
		}
		    
		//Interpolate() Tests
		
		for(i=1;i<=10;++i){
		  c=.1*double(i);
		  a=Interpolate(pa,pb,c);
		  TEST((a.x())==(pb.x()*c+pa.x()*(1.0-c)));
		  TEST((a.y())==(pb.y()*c+pa.y()*(1.0-c)));
		  TEST((a.z())==(pb.z()*c+pa.z()*(1.0-c)));
		}
		
		//InInterval Tests
		

		double epsilon,dist;

		for(i=1;i<=100;++i){
		  epsilon=i;
		  X=(pa.x()-pb.x());
		  Y=(pa.y()-pb.y());
		  Z=(pa.z()-pb.z());
		  dist=Max(Abs(pa.x()-pb.x()),Abs(pa.y()-pb.y()),Abs(pa.z()-pb.z()));
		  dist/=2;	
		  if(dist<epsilon){
		    TEST(pa.InInterval(pb,epsilon));
		   

		  }
		  if(dist>=epsilon+1.e-6)
		    TEST(!(pa.InInterval(pb,epsilon)));
		}
	    }    
	}
    }
}



#endif

