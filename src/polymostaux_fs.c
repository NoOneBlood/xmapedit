// Generated by bin2c. Do not edit directly.

const char default_polymostaux_fs_glsl[] =
    "#glbuild(ES2) #version 100\r\n"
    "#glbuild(2)   #version 110\r\n"
    "#glbuild(3)   #version 140\r\n"
    "\r\n"
    "#ifdef GL_ES\r\n"
    "precision lowp float;\r\n"
    "precision lowp int;\r\n"
    "#  define o_fragcolour gl_FragColor\r\n"
    "#elif __VERSION__ < 140\r\n"
    "#  define mediump\r\n"
    "#  define o_fragcolour gl_FragColor\r\n"
    "#else\r\n"
    "#  define varying in\r\n"
    "#  define texture2D texture\r\n"
    "out vec4 o_fragcolour;\r\n"
    "#endif\r\n"
    "\r\n"
    "uniform sampler2D u_texture;\r\n"
    "uniform vec4 u_colour;\r\n"
    "uniform vec4 u_bgcolour;\r\n"
    "uniform int u_mode;\r\n"
    "\r\n"
    "varying mediump vec2 v_texcoord;\r\n"
    "\r\n"
    "void main(void)\r\n"
    "{\r\n"
    "    vec4 pixel;\r\n"
    "\r\n"
    "    if (u_mode == 0) {\r\n"
    "        // Text.\r\n"
    "        pixel = texture2D(u_texture, v_texcoord);\r\n"
    "        o_fragcolour = mix(u_bgcolour, u_colour, pixel.a);\r\n"
    "    } else if (u_mode == 1) {\r\n"
    "        // Tile screen.\r\n"
    "        pixel = texture2D(u_texture, v_texcoord);\r\n"
    "        o_fragcolour = mix(u_bgcolour, pixel, pixel.a);\r\n"
    "    } else if (u_mode == 2) {\r\n"
    "        // Foreground colour.\r\n"
    "        o_fragcolour = u_colour;\r\n"
    "    }\r\n"
    "}\r\n"
;

const int default_polymostaux_fs_glsl_size = 955;