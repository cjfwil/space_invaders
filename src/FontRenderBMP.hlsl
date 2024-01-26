cbuffer view_info : register(b0)
{
    matrix view;
    float2 offset;
    float2 scale;
    float rot;    
}

Texture2D fontTexture : register(t0);
SamplerState samplerState : register(s0);

struct ps_input {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

ps_input vs_main(float2 pos: POSITION, float2 uv: TEXCOORD)
{
    pos *= scale;

    float2x2 rotMatrix = float2x2(cos(rot), -sin(rot),
                                  sin(rot), cos(rot));
    pos = mul(pos, rotMatrix);
    pos += offset;

    ps_input output;
    output.pos = mul(float4(pos, 0.0f, 1.0f), view);
    output.uv = uv;
    return output;
}

float4 ps_main(ps_input input) : SV_TARGET
{
    float4 clr = fontTexture.Sample(samplerState, input.uv);
    //clip(clr.a < 0.1f ? -1 : 1);
    //clr = float4(0.0f, 1.0f, 1.0f, 1.0f);
    return (clr);
}
