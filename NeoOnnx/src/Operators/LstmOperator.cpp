/* Copyright © 2017-2020 ABBYY Production LLC

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
--------------------------------------------------------------------------------------------------------------*/

#include "../common.h"
#pragma hdrstop

#include "LstmOperator.h"
#include "NeoOnnxCheck.h"
#include "TensorUtils.h"

#include "onnx.pb.h"

namespace NeoOnnx {

CLstmOperator::CLstmOperator( const onnx::NodeProto& lstm, int opsetVersion ) :
	CLayerOperator( lstm, opsetVersion ),
	direction( "forward" ),
	hiddenSize( -1 )
{
	// v1 - original
	// v7 - added initial state and peephole weight inputs
	CheckNeoOnnxSupport( OpsetVersion >= 1 && OpsetVersion <= MaxOpsetVersion, "opset version", *this );

	CheckOnnxProtocol( InputCount() >= 3 && InputCount() <= 8, "operator must have from 3 upto 8 inputs", *this );
	CheckOnnxProtocol( OutputCount() >= 1 && OutputCount() <= 3, "operator must have from 1 upto 3 outputs", *this );

	GetAttribute( "direction", direction );
	CheckNeoOnnxSupport( direction != "bidirectional", "bidirectional LSTM", *this );

	CheckOnnxProtocol( GetAttribute( "hidden_size", hiddenSize ), "'hidden_size' attribute is missing", *this );
}

void CLstmOperator::AddLayers( const CTensorArray& inputs, CDnn& dnn, CTensorArray& outputs ) const
{
	CheckOnnxProtocol( inputs[0] != nullptr, "input can't be optional", *this );

	const CTensorShape& inputShape = inputs[0]->Shape();

	CPtr<CDnnBlob> bias = nullptr;
	if( InputCount() > 3 && inputs[3] != nullptr ) {
		CheckNeoOnnxSupport( inputs[3]->IsCalculated(), "User-provided bias", *this );
		bias = dynamic_cast<const CDataTensor*>( inputs[3].Ptr() )->Data()->GetCopy();
	}

	// NeoML doesn't support sequence lengths
	CheckNeoOnnxSupport( InputCount() <= 4 || inputs[4] == nullptr, "sequence lengths", *this );

	// NeoML doesn't support peepholes
	CheckNeoOnnxSupport( InputCount() <= 7 || inputs[7] == nullptr, "peepholes", *this );

	CPtr<const CUserTensor> inputData = AsUserTensor(
		*ConvertTensor( *inputs[0], CTensorLayout( { BD_BatchLength, BD_BatchWidth, BD_Channels } ) ),
		Name() + "_Source", dnn );

	IMathEngine& mathEngine = dnn.GetMathEngine();
	CPtr<CLstmLayer> lstmLayer = new CLstmLayer( mathEngine );
	lstmLayer->SetName( Name() );

	CheckNeoOnnxSupport( inputs[1] != nullptr && inputs[1]->IsCalculated(), "User-provided weight", *this );
	CPtr<CDnnBlob> weights = dynamic_cast<const CDataTensor*>( inputs[1].Ptr() )->Data()->GetCopy();

	const int inputObjectSize = inputs[1]->Shape()[2];

	CBlobDesc blobDesc( CT_Float );
	blobDesc.SetDimSize( BD_BatchWidth, 4 * hiddenSize );
	blobDesc.SetDimSize( BD_Channels, inputObjectSize );
	weights->ReinterpretDimensions( blobDesc );
	blobDesc.SetDimSize( BD_Channels, hiddenSize );
	
	CheckNeoOnnxSupport( inputs[2] != nullptr && inputs[2]->IsCalculated(), "User-provided recurrent weight", *this );
	CPtr<CDnnBlob> recurWeights = dynamic_cast<const CDataTensor*>( inputs[2].Ptr() )->Data()->GetCopy();
	recurWeights->ReinterpretDimensions( blobDesc );

	lstmLayer->SetHiddenSize( hiddenSize );
	weights = reorderGates( weights, BD_BatchWidth );
	recurWeights = reorderGates( recurWeights, BD_BatchWidth );

	lstmLayer->SetInputWeightsData( weights );
	lstmLayer->SetRecurWeightsData( recurWeights );

	if( bias != nullptr ) {
		CPtr<CDnnBlob> weightBias = CDnnBlob::CreateDataBlob( mathEngine, CT_Float, 1, 1, 4 * hiddenSize );
		CPtr<CDnnBlob> recurBias = weightBias->GetClone();

		mathEngine.VectorCopy( weightBias->GetData(), bias->GetData(), 4 * hiddenSize );
		mathEngine.VectorCopy( recurBias->GetData(), bias->GetData() + 4 * hiddenSize, 4 * hiddenSize );

		weightBias = reorderGates( weightBias, BD_Channels );
		recurBias = reorderGates( recurBias, BD_Channels );

		lstmLayer->SetInputFreeTermData( weightBias );
		lstmLayer->SetRecurFreeTermData( recurBias );
	}

	lstmLayer->Connect( 0, *inputData->Layer(), inputData->OutputIndex() );
	dnn.AddLayer( *lstmLayer );

	outputs.Add( new CUserTensor( { inputShape[0], 1, inputShape[1], hiddenSize },
		CTensorLayout( { BD_BatchLength, BD_ListSize, BD_BatchWidth, BD_Channels } ),
		CLayerOutput( lstmLayer, 0 ) ) );
	outputs.Add( nullptr, OutputCount() - 1 );
}

// Converts lstm weights from onnx format to NeoML
// because gate weights in onnx and NeoML are oredered differently
CPtr<CDnnBlob> CLstmOperator::reorderGates( CPtr<CDnnBlob> blob, TBlobDim dim ) const
{
	IMathEngine& mathEngine = blob->GetMathEngine();

	CObjectArray<CDnnBlob> onnxGateWeight;

	CBlobDesc gateWeightDesc = blob->GetDesc();
	gateWeightDesc.SetDimSize( dim, gateWeightDesc.DimSize( dim ) / 4 );
	
	for( int gateIndex = 0; gateIndex < 4; ++gateIndex ) {
		onnxGateWeight.Add( CDnnBlob::CreateBlob( mathEngine, gateWeightDesc ) );
	}

	CDnnBlob::SplitByDim( mathEngine, dim, blob, onnxGateWeight );

	CObjectArray<CDnnBlob> neoMLGateWeight;
	// Main gate
	neoMLGateWeight.Add( onnxGateWeight[3] );
	// Forget gate
	neoMLGateWeight.Add( onnxGateWeight[2] );
	// Input gate
	neoMLGateWeight.Add( onnxGateWeight[0] );
	// Reset gate
	neoMLGateWeight.Add( onnxGateWeight[1] );

	CPtr<CDnnBlob> result = blob->GetClone();
	CDnnBlob::MergeByDim( mathEngine, dim, neoMLGateWeight, result );
	return result;
}

} // namespace NeoOnnx
