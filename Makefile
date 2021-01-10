# Vulkan Code

SRC	=	main.cpp \
		VulkanRenderer.cpp \

OBJ	=	$(SRC:.cpp=.o)

# Shaders

SHADERS	=	shader1 shader2 shader3

FRAGS	=	$(addsuffix .frag, $(SHADERS))
VERTS	=	$(addsuffix .vert, $(SHADERS))

# C++ Flags

CFLAGS	=	-std=c++17 -O2 -g
LDFLAGS	=	-lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi
NAME	=	vulkanTest

all: $(OBJ)
	g++ $(CFLAGS) -o $(NAME) $(OBJ) $(LDFLAGS)

.PHONY: test clean

test: all
	./$(NAME)

clean:
	rm -f $(NAME)

fclean: clean
	rm -f $(OBJ)

re: fclean all

shaders:
	cd Shaders && for SHADER in $(SHADERS); do glslangValidator -V $$SHADER.vert -o $$SHADER\_vert.spv; glslangValidator -V $$SHADER.frag -o $$SHADER\_frag.spv; done
