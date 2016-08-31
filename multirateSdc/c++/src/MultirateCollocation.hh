#ifndef MULTIRATE_COLLOCATION_HH
#define MULTIRATE_COLLOCATION_HH

#include <array>
#include <fstream>
#include <sstream>
#include <string>
#include <cassert>
#include <vector>
#include <iostream>

namespace Helper
{
	template<std::size_t M>
	struct LagrangePoly
	{
		typedef std::array<double, M> Vec;
		Vec nodes;
		double fac;
		unsigned q;
		LagrangePoly(const Vec& nodes, unsigned q):
			nodes(nodes), q(q)
		{
			fac=1.0;
			for(unsigned i(0); i < q; ++i)
				fac *= (nodes[q]-nodes[i]);
			for(unsigned i(q+1); i < M; ++i)
				fac *= (nodes[q]-nodes[i]);
			fac = 1.0/fac;
		}
	};

	struct MonomPoly
	{
		std::vector<double > coeffs;
		unsigned degree;

		MonomPoly(unsigned deg):
			coeffs(deg+1),
			degree(deg)
		{}
		

		double operator()(const double& t) const
		{
			double ret(coeffs[0]);
			for(unsigned i(1); i <= degree; ++ i)
				ret = ret*t + coeffs[i];
			return ret;
		}

		double integrate(double t0, double t1)
		{
			MonomPoly stamm(degree+1);
			for(unsigned i(0); i < degree; ++i) {
				stamm.coeffs[i] = coeffs[i]/(degree-i+1);
			}
			stamm.coeffs[degree] = coeffs[degree];
			stamm.coeffs[degree+1] = 0.0;
			return stamm(t1)-stamm(t0);
		}
	};

	MonomPoly operator*(const MonomPoly& l, const MonomPoly& r)
	{
		MonomPoly ret(l.degree+r.degree);
		for(auto& d : ret.coeffs) d= 0.0;
		for(unsigned q(0); q <= l.degree; ++q)
		{
			for(unsigned p(0); p <= r.degree; ++p)
				ret.coeffs[q+p] += l.coeffs[q]*r.coeffs[p];
		}
		return ret;
	}

	template<std::size_t M>
	MonomPoly toMonom(const LagrangePoly<M>& lPoly)
	{
		MonomPoly ret(0);
		ret.coeffs[0] = 1.0;
		MonomPoly mulPoly(1); mulPoly.coeffs[0]=1.0; 
		for(unsigned i(0); i < lPoly.nodes.size(); ++i) {
			if(i==lPoly.q)
				continue;
			mulPoly.coeffs[1] = -lPoly.nodes[i];
			ret = ret*mulPoly;
		}
		for(auto& d: ret.coeffs) d*=lPoly.fac;
		return ret;
	}
}
template<unsigned M >
struct Collocation
{
	typedef std::array<std::array<double, M>, M> Mat;
	typedef std::array<std::array<double, M>, M+2> Mat2;
	typedef std::array<double, M> Vec;

	Vec delta_m, nodes;
	Mat sMat;
	double tleft;

	void set(const Mat2& data, double t0, double t1)
	{
		double dt=t1-t0;
		assert(dt > 0);
		for(unsigned mr(0); mr < M; ++mr) {
			for(unsigned mc(0); mc < M; ++mc) {
				sMat[mr][mc] = dt*data[mr][mc];
			}
			nodes[mr] = t0+dt*data[M][mr];
			delta_m[mr] = mr == 0 ? nodes[mr] : nodes[mr]-nodes[mr-1];
		}
		tleft = t0;
	}

	template<typename F >
	Vec evalAtNodes(const F& f) const
	{
		Vec ret;
		for(unsigned i(0); i < M; ++i)
			ret[i] = f(nodes[i]);
		return ret;
	}
};

template< unsigned M, unsigned P >
struct MultirateCollocation
{
	typedef std::array<std::array<double, M>, M> Mat;
	typedef std::array<std::array<double, M>, M+2> MatMp2;
	typedef std::array<std::array<double, P>, P+2> MatPp2;

	template<std::size_t S>
	void readMatrix(std::array<std::array<double, S>, S+2>& dest, std::string fname)
	{
		std::fstream file(fname);
		std::string line;
		unsigned r(0);
		getline(file, line);
		while(file.good() || line.length() > 0) {
			std::stringstream sl;
			sl << line;
			double col;
			unsigned j(0); 
			sl >> col;
			while(sl.good()) {
				dest[r][j] = col;
				++j;
				sl >> col;
			}
			assert(j == S);
			++r;
			line.clear();
			getline(file, line);
		}
		assert(r == S+2);
		file.close();
	}

#if 0
	template<std::size_t S >
	void print(std::array<std::array<double, S>, S+2>& mat)
	{
		std::cout << "[";
		for(auto& d:mat) {
			std::cout << "["; for(auto& d2:d) std::cout << " " << d2;
			std::cout << "]\n";
		}
		std::cout << "]\n";
	}
#endif

	void setInterval(double t0, double t1)
	{
		coll.set(sMat_M, t0, t1);
		coll_sub[0].set(sMat_P, coll.tleft, coll.nodes[0]);
		for(unsigned m(1); m < M; ++m) {
			coll_sub[m].set(sMat_P, coll.nodes[m-1], coll.nodes[m]);
		}

		for(unsigned mr(0); mr < M; ++mr)
			for(unsigned mc(0); mc < M; ++mc)
				Shat_mp[mr][mc] = 0.0;
		for(unsigned m(0); m < M; ++m) {
			for(unsigned j(0); j < P; ++j)
				for(unsigned p(0); p < P; ++p)
					Shat_mp[m][j] += coll_sub[m].sMat[p][j];
		}

		for(unsigned m(0); m < M; ++m)
			for(unsigned q(0); q < P; ++q) {
				Helper::LagrangePoly<M> lPoly(coll.nodes, q);
				Helper::MonomPoly monPoly(toMonom(lPoly));
				for(unsigned p(0); p < P; ++p) {
					double ta = p==0 ? coll_sub[m].tleft : coll_sub[m].nodes[p-1];
					S_mnp[m][q][p] = monPoly.integrate(ta, coll_sub[m].nodes[p]);
					//std::cout << m << " " << p << " " << q << " " << S_mnp[m][p][q] << std::endl;
				}
			}
		/*
		S_mnp[0][0]={ 0.22916667,  0.1875    };
		S_mnp[0][1]={-0.0625  ,   -0.02083333};
		S_mnp[1][0]={ 0.25   ,     0.08333333};
		S_mnp[1][1]={ 0.08333333 , 0.25      };*/
	}

	MultirateCollocation()
	{
		readMatrix(sMat_M, "sdc_quad_weights/radau_right-M2.dat");
		readMatrix(sMat_P, "sdc_quad_weights/equi_noleft-M2.dat");
		setInterval(0.0, 1.0);
#if 0
		print(sMat_M);
		print(sMat_P);
#endif
	
	}
	

	template<typename Vec >
	void integrate_m_mp1(const std::array<Vec, M>& fu, unsigned m, Vec& res)
	{
		setValue(res, 0.0);
		for(unsigned j(0); j < M; ++j) {
			axpy(coll.sMat[m][j], fu[j], res);
		}
	}

	template<typename Vec >
	void integrate_m_mp1_sub(const std::array<Vec, P>& fu_sub, unsigned m, Vec& iVal)
	{
		setValue(iVal, 0.0);
		for(unsigned p(0); p < P; ++p)
			axpy(Shat_mp[m][p], fu_sub[p], iVal);
	}

	template<typename Vec >
	void integrate_p_pp1(std::array<Vec, M>& fu, unsigned m, unsigned p, Vec& iVal)
	{
		setValue(iVal, 0.0);
		for(unsigned n(0); n < M; ++n) {
			axpy(S_mnp[m][n][p] ,fu[n], iVal);
		}

	}

	template<typename Vec >
	void integrate_p_pp1_sub(const std::array<Vec, P>& fu_sub , unsigned m, unsigned p, Vec& iVal)
	{
		setValue(iVal, 0.0);
		const typename Collocation<M>::Mat& sMat(coll_sub[m].sMat);
		for(unsigned j(0); j < P; ++j) {
			axpy( sMat[p][j], fu_sub[j], iVal);
		}
	}

	Collocation<M> coll;
	std::array<Collocation<P>, M> coll_sub;
	private:
	Mat Shat_mp;
	std::array<Mat, M> S_mnp;
	MatMp2 sMat_M;
	MatPp2 sMat_P;


};
#endif
