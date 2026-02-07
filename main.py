import math
import random
from dataclasses import dataclass

import pygame

WINDOW_WIDTH = 960
WINDOW_HEIGHT = 540
TILE_SIZE = 16
CHUNK_SIZE = 32
SEED = 1337


def lerp(a, b, t):
    return a + (b - a) * t


def smoothstep(t):
    return t * t * (3 - 2 * t)


def hash2d(x, y, seed):
    value = (x * 374761393 + y * 668265263 + seed * 1442695040888963407) & 0xFFFFFFFF
    value = (value ^ (value >> 13)) * 1274126177 & 0xFFFFFFFF
    return (value ^ (value >> 16)) / 0xFFFFFFFF


def value_noise(x, y, seed):
    x0 = math.floor(x)
    y0 = math.floor(y)
    x1 = x0 + 1
    y1 = y0 + 1

    sx = smoothstep(x - x0)
    sy = smoothstep(y - y0)

    n00 = hash2d(x0, y0, seed)
    n10 = hash2d(x1, y0, seed)
    n01 = hash2d(x0, y1, seed)
    n11 = hash2d(x1, y1, seed)

    nx0 = lerp(n00, n10, sx)
    nx1 = lerp(n01, n11, sx)
    return lerp(nx0, nx1, sy)


def fractal_noise(x, y, seed, octaves=4, lacunarity=2.0, gain=0.5):
    frequency = 1.0
    amplitude = 1.0
    total = 0.0
    max_value = 0.0
    for _ in range(octaves):
        total += value_noise(x * frequency, y * frequency, seed) * amplitude
        max_value += amplitude
        amplitude *= gain
        frequency *= lacunarity
    return total / max_value


@dataclass
class Tile:
    block_id: int


class Chunk:
    def __init__(self, cx, cy, seed):
        self.cx = cx
        self.cy = cy
        self.seed = seed
        self.tiles = [[Tile(0) for _ in range(CHUNK_SIZE)] for _ in range(CHUNK_SIZE)]
        self.generate()

    def generate(self):
        for y in range(CHUNK_SIZE):
            for x in range(CHUNK_SIZE):
                world_x = self.cx * CHUNK_SIZE + x
                world_y = self.cy * CHUNK_SIZE + y
                height = fractal_noise(world_x / 24.0, world_y / 24.0, self.seed)
                moisture = fractal_noise(world_x / 48.0 + 100, world_y / 48.0 - 100, self.seed + 42)
                if height < 0.35:
                    block_id = 3  # water
                elif height < 0.45:
                    block_id = 2  # sand
                else:
                    block_id = 1 if moisture > 0.45 else 4
                self.tiles[y][x] = Tile(block_id)

    def get_tile(self, lx, ly):
        return self.tiles[ly][lx]


class World:
    def __init__(self, seed):
        self.seed = seed
        self.chunks = {}

    def get_chunk(self, cx, cy):
        key = (cx, cy)
        if key not in self.chunks:
            self.chunks[key] = Chunk(cx, cy, self.seed)
        return self.chunks[key]

    def get_tile(self, wx, wy):
        cx = math.floor(wx / CHUNK_SIZE)
        cy = math.floor(wy / CHUNK_SIZE)
        lx = int(wx - cx * CHUNK_SIZE)
        ly = int(wy - cy * CHUNK_SIZE)
        chunk = self.get_chunk(cx, cy)
        return chunk.get_tile(lx, ly)


class Camera:
    def __init__(self, x=0.0, y=0.0):
        self.x = x
        self.y = y

    def move(self, dx, dy):
        self.x += dx
        self.y += dy


def draw_world(screen, world, camera):
    tiles_x = WINDOW_WIDTH // TILE_SIZE + 2
    tiles_y = WINDOW_HEIGHT // TILE_SIZE + 2
    start_x = int(math.floor(camera.x))
    start_y = int(math.floor(camera.y))

    for y in range(tiles_y):
        for x in range(tiles_x):
            world_x = start_x + x
            world_y = start_y + y
            tile = world.get_tile(world_x, world_y)
            screen_x = x * TILE_SIZE - int((camera.x - start_x) * TILE_SIZE)
            screen_y = y * TILE_SIZE - int((camera.y - start_y) * TILE_SIZE)

            if tile.block_id == 1:
                color = (80, 180, 80)
            elif tile.block_id == 2:
                color = (200, 190, 120)
            elif tile.block_id == 3:
                color = (70, 120, 200)
            else:
                color = (120, 120, 120)

            pygame.draw.rect(
                screen,
                color,
                pygame.Rect(screen_x, screen_y, TILE_SIZE, TILE_SIZE),
            )


def main():
    pygame.init()
    pygame.display.set_caption("Minecraft Infdev Prototype")
    screen = pygame.display.set_mode((WINDOW_WIDTH, WINDOW_HEIGHT))
    clock = pygame.time.Clock()

    world = World(SEED)
    camera = Camera(0.0, 0.0)
    running = True

    while running:
        dt = clock.tick(60) / 1000.0
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False

        keys = pygame.key.get_pressed()
        speed = 8.0
        dx = (keys[pygame.K_d] - keys[pygame.K_a]) * speed * dt
        dy = (keys[pygame.K_s] - keys[pygame.K_w]) * speed * dt
        camera.move(dx, dy)

        screen.fill((20, 20, 30))
        draw_world(screen, world, camera)
        pygame.display.flip()

    pygame.quit()


if __name__ == "__main__":
    main()
