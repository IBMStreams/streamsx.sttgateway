/*
 * WatsonSTTImpl.hpp
 *
 * Licensed Materials - Property of IBM
 * Copyright IBM Corp. 2019, 2020
 *
 *  Created on: Jan 14, 2020
 *      Author: joergboe
*/

#ifndef COM_IBM_STREAMS_STTGATEWAY_WATSONSTTIMPL_HPP_
#define COM_IBM_STREAMS_STTGATEWAY_WATSONSTTIMPL_HPP_

namespace com { namespace ibm { namespace streams { namespace sttgateway {

/*
 * Implementation class for operator Watson STT
 * Move almost of the c++ code of the operator into this class to take the advantage of c++ editor support
 *
 * Template argument : Operator type (must inherit from SPL::Operator)
 */
template<typename OP>
class WatsonSTTImpl {
public:
	WatsonSTTImpl(OP & splOperator_);
	WatsonSTTImpl(WatsonSTTImpl const &) = delete;
private:
	OP & splOperator;

};

template<typename OP>
WatsonSTTImpl<OP>::WatsonSTTImpl(OP & splOperator_) :
	splOperator(splOperator_)
{ }

}}}}

#endif /* COM_IBM_STREAMS_STTGATEWAY_WATSONSTTIMPL_HPP_ */
