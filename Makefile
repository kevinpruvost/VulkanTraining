SRC	=	main.cpp \
		VulkanRenderer.cpp \


OBJ	=	$(SRC:.cpp=.o)

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