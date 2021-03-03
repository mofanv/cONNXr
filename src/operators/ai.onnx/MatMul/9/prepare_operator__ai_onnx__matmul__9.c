//this file was generated by ../../../../../../scripts/onnx_generator/OperatorTemplate.py
#include "operator__ai_onnx__matmul__9.h"
#include "tracing.h"
#include "utils.h"

operator_status
prepare_operator__ai_onnx__matmul__9(
    node_context *ctx
)
{
    TRACE_ENTRY(1);

    TRACE_NODE(2, true, ctx->onnx_node);

    /* UNCOMMENT AS NEEDED */

    Onnx__TensorProto *i_A = searchInputByName(ctx, 0);
    Onnx__TensorProto *i_B = searchInputByName(ctx, 1);

    TRACE_TENSOR(2, true, i_A);
    TRACE_TENSOR(2, true, i_B);

    Onnx__TensorProto *o_Y = searchOutputByName(ctx, 0);

    /* ALLOCATE AND INITIALIZE CONTEXT HERE IF NEEDED */

    // context_operator__ai_onnx__matmul__9 *op_ctx = NULL;
    // op_ctx = malloc(sizeof(context_operator__ai_onnx__matmul__9));
    // TRACE_FATAL(0 , !op_ctx, "could not allocate executer_context");

    /* INITIALIZE OUTPUTS DATA_TYPE AND SHAPE HERE */

    // TODO Hardcoded for 2 dimensions
    // TODO Might be useful to define a macro like
    // #define I(a,b,c,d) I[(a)+(b)*oH+(c)*oH*oW+(d)*oH*oW*C]
    // dont know how to handle the different dimensions though
    TRACE_FATAL(0, i_A->n_dims != 2 || i_B->n_dims != 2, "not supported dimensions");

    o_Y->has_raw_data = 0;
    o_Y->n_dims       = 2;
    o_Y->dims         = malloc(o_Y->n_dims*sizeof(int64_t));
    o_Y->dims[0]      = i_A->dims[0];
    o_Y->dims[1]      = i_B->dims[1];
    o_Y->data_type    = i_A->data_type;

    /* MALLOC OUTPUT TENSORS HERE */

    mallocTensorData(o_Y);

    TRACE_TENSOR(2, true, o_Y);

    /* CHOOSE EXECUTER AND CONTEXT HERE */
    /* YOU MAY USE THE GENERATED RESOLVER */

    ctx->executer = resolve_operator__ai_onnx__matmul__9(ctx);
    // ctx->executer_context = op_ctx;

    TRACE_EXIT(1);

    /* CHANGE RETURN CODE IF THIS PREPARER IS VALID */
    // return OP_ENOSYS;
    return OP_OK;
}