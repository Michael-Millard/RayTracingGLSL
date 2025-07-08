#version 430 core

out vec4 FragColor;
in vec2 TexCoords;

uniform mat4 projection;
uniform mat4 view;
uniform samplerCube skybox;
uniform float modelIOR;    
uniform bool reflectEnable;

const float airIOR = 1.0;

// SSBO binding
layout(std430, binding = 0) buffer Triangles 
{
    // Each 3 consecutive entries = 1 triangle (v0, v1, v2), each in homogeneous coords (x, y, z, w) with normals (n0, n1, n2)
    vec4 triangles[]; 
};

// Fresnel-Schlick approximation
float fresnelSchlick(float cosTheta, float F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

// Möller–Trumbore ray-triangle intersection
bool intersectTriangle(vec3 orig, vec3 dir,
    vec3 v0, vec3 v1, vec3 v2,
    out float t, out float u, out float v)
{
    const float EPSILON = 1e-5;
    vec3 edge1 = v1 - v0;
    vec3 edge2 = v2 - v0;
    vec3 h = cross(dir, edge2);
    float a = dot(edge1, h);
    if (abs(a) < EPSILON) 
        return false;

    float f = 1.0 / a;
    vec3 s = orig - v0;
    u = f * dot(s, h);
    if (u < 0.0 || u > 1.0) 
        return false;

    vec3 q = cross(s, edge1);
    v = f * dot(dir, q);
    if (v < 0.0 || (u + v) > 1.0) 
        return false;

    t = f * dot(edge2, q);
    return t > EPSILON;
}

void main()
{
    // Reconstruct ray from screen UV
    vec2 ndc = TexCoords * 2.0 - 1.0;
    vec4 clip = vec4(ndc, -1.0, 1.0); // z = -1 points into screen
    vec4 viewPos = inverse(projection) * clip;
    viewPos /= viewPos.w;
    vec3 rayDirView = normalize(viewPos.xyz);
    vec3 rayOrigin = vec3(inverse(view) * vec4(0.0, 0.0, 0.0, 1.0)); // Camera world pos
    vec3 rayDir = normalize(vec3(inverse(view) * vec4(rayDirView, 0.0)));

    // Prepare for bounce loop
    int maxBounces = 4;
    vec3 origin = rayOrigin;
    vec3 dir = rayDir;
    float currentIOR = airIOR;

    // Used after loop breaks
    vec3 N = vec3(0.0);
    vec3 color = vec3(0.0);
    for (int bounce = 0; bounce < maxBounces; ++bounce)
    {
        // Search for closest triangle hit
        float minT = 1e20;
        vec3 hitNormal, hitPoint;
        bool hit = false;

        for (uint i = 0; i + 5 < triangles.length(); i += 6)
        {
            // Triangle vertices (first 3 elements)
            vec3 v0 = triangles[i].xyz;
            vec3 v1 = triangles[i + 1].xyz;
            vec3 v2 = triangles[i + 2].xyz;

            float t, u, v;
            if (intersectTriangle(origin, dir, v0, v1, v2, t, u, v) && t < minT)
            {
                minT = t;
                hit = true;

                // Triangle normals (last 3 elements)
                vec3 n0 = triangles[i + 3].xyz;
                vec3 n1 = triangles[i + 4].xyz;
                vec3 n2 = triangles[i + 5].xyz;

                float w = 1.0 - u - v;
                hitNormal = normalize(w * n0 + u * n1 + v * n2);
                hitPoint = origin + t * dir;
            }
        }

        // If missed, break and let last direction index skybox
        if (!hit)
            break;

        // Face the normal against the incoming ray
        N = faceforward(hitNormal, dir, hitNormal);

        // Refraction alternates between air and model
        float nextIOR = (abs(currentIOR - airIOR) < 0.001) ? modelIOR : airIOR;
        vec3 T = refract(dir, N, currentIOR / nextIOR);

        if (length(T) < 0.001)
        {
            // Total internal reflection: fallback to reflection
            dir = reflect(dir, N);
        }
        else
        {
            dir = normalize(T);
            currentIOR = nextIOR;
        }

        // Move ray origin slightly forward to avoid self-hit
        origin = hitPoint + dir * 0.001;
    }

    // Mix with reflection if enabled
    if (reflectEnable)
    {
        float cosTheta = clamp(dot(-dir, N), 0.0, 1.0);
        float F0 = pow((airIOR - modelIOR) / (airIOR + modelIOR), 2.0);
        float fresnel = fresnelSchlick(cosTheta, F0);
        vec3 reflectedColor = texture(skybox, reflect(dir, N)).rgb;
        vec3 refractedColor = texture(skybox, dir).rgb;
        color = mix(refractedColor, reflectedColor, fresnel);
    }
    else
        color = texture(skybox, dir).rgb;

    FragColor = vec4(color, 1.0);
}
