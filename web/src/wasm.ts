/**
 * wasm.ts — TypeScript wrapper around the Emscripten-compiled WASM module.
 *
 * Provides a clean async API used by React components.  The CSV dataset is
 * fetched at runtime from a well-known URL and fed into the C++ engine via
 * `wasm_load_csv`.  All search / browse / user-data operations go through
 * the C++ code running as WebAssembly.
 */

// The Emscripten factory function is loaded as a global via <script> in
// index.html. We declare its type here.
declare function createStreamingModule(opts?: Record<string, unknown>): Promise<EmscriptenModule>;

interface EmscriptenModule {
  ccall: (
    name: string,
    returnType: string | null,
    argTypes: string[],
    args: unknown[],
  ) => unknown;
  cwrap: (
    name: string,
    returnType: string | null,
    argTypes: string[],
  ) => (...args: unknown[]) => unknown;
}

// ── Singleton module instance ───────────────────────────────────────────────
let modulePromise: Promise<EmscriptenModule> | null = null;

async function getModule(): Promise<EmscriptenModule> {
  if (!modulePromise) {
    modulePromise = createStreamingModule({
      locateFile: (path: string) => {
        // Vite serves from /public during dev; in production the base path
        // is injected by vite.config.ts.
        const base = import.meta.env.BASE_URL ?? '/';
        return `${base}${path}`;
      },
    });
  }
  return modulePromise;
}

// ── Public types ────────────────────────────────────────────────────────────
export interface Movie {
  id: number;
  year: number;
  title: string;
  origin: string;
  director: string;
  cast: string;
  genre: string;
  wiki_url: string;
  plot: string;
  tags: string[];
}

export interface SearchParams {
  text?: string;
  genre?: string;
  tag?: string;
  yearFrom?: number;
  yearTo?: number;
  offset?: number;
  limit?: number;
}

export interface UserData {
  updatedAt?: string;
  totalWatchLater: number;
  totalLiked: number;
  watchLater: MovieData[];
  liked: MovieData[];
}

export interface MovieData {
  id: string;
  title: string;
  releaseYear: number;
  origin: string;
  director: string;
  cast: string;
  genre: string;
  wikiPage: string;
  plot: string;
  addedAt: string;
}

// ── CSV data URL (clean dataset) ────────────────────────────────────────────
// This URL should point to the cleaned CSV. During development you can drop
// data_clean.csv into web/public/.  In CI, the deploy workflow can copy it.
const CSV_URL = (() => {
  const base = import.meta.env.BASE_URL ?? '/';
  return `${base}data_clean.csv`;
})();

// ── Engine API ──────────────────────────────────────────────────────────────

/**
 * Initialise the engine: download CSV → pass to C++ → build indexes.
 * Returns the number of movies loaded.
 */
export async function initEngine(): Promise<number> {
  const mod = await getModule();

  // Fetch CSV dataset
  const resp = await fetch(CSV_URL);
  if (!resp.ok) throw new Error(`Failed to fetch CSV: ${resp.statusText}`);
  const csvText = await resp.text();

  // Pass to C++
  const count = mod.ccall('wasm_load_csv', 'number', ['string'], [csvText]) as number;
  if (count < 0) throw new Error('wasm_load_csv returned error');
  return count;
}

export async function isReady(): Promise<boolean> {
  const mod = await getModule();
  return mod.ccall('wasm_is_ready', 'boolean', [], []) as boolean;
}

export async function movieCount(): Promise<number> {
  const mod = await getModule();
  return mod.ccall('wasm_movie_count', 'number', [], []) as number;
}

export async function search(params: SearchParams): Promise<Movie[]> {
  const mod = await getModule();
  const json = mod.ccall('wasm_search', 'string', [
    'string', 'string', 'string', 'number', 'number', 'number', 'number',
  ], [
    params.text ?? '',
    params.genre ?? '',
    params.tag ?? '',
    params.yearFrom ?? 0,
    params.yearTo ?? 0,
    params.offset ?? 0,
    params.limit ?? 20,
  ]) as string;
  return JSON.parse(json) as Movie[];
}

export async function getMovie(id: number): Promise<Movie | null> {
  const mod = await getModule();
  const json = mod.ccall('wasm_get_movie', 'string', ['number'], [id]) as string;
  const parsed = JSON.parse(json);
  return parsed ?? null;
}

export async function getGenres(): Promise<string[]> {
  const mod = await getModule();
  const json = mod.ccall('wasm_get_genres', 'string', [], []) as string;
  return JSON.parse(json) as string[];
}

export async function getFeatured(count = 12): Promise<Movie[]> {
  const mod = await getModule();
  const json = mod.ccall('wasm_get_featured', 'string', ['number'], [count]) as string;
  return JSON.parse(json) as Movie[];
}

export async function browseGenre(genre: string, offset = 0, limit = 20): Promise<Movie[]> {
  const mod = await getModule();
  const json = mod.ccall('wasm_browse_genre', 'string', [
    'string', 'number', 'number',
  ], [genre, offset, limit]) as string;
  return JSON.parse(json) as Movie[];
}

// ── User Data API (likes, watch later) ──────────────────────────────────────

export async function getUserData(): Promise<UserData> {
  const mod = await getModule();
  const json = mod.ccall('wasm_get_user_data_json', 'string', [], []) as string;
  return JSON.parse(json) as UserData;
}

export async function loadUserData(json: string): Promise<boolean> {
  const mod = await getModule();
  return mod.ccall('wasm_load_user_data_json', 'boolean', ['string'], [json]) as boolean;
}

export async function toggleWatchLater(movie: Movie): Promise<boolean> {
  const mod = await getModule();
  return mod.ccall('wasm_toggle_watch_later', 'boolean', [
    'string', 'number', 'string', 'string', 'string', 'string',
  ], [
    movie.title, movie.year, movie.genre, movie.director, movie.plot, movie.wiki_url,
  ]) as boolean;
}

export async function hasWatchLater(title: string): Promise<boolean> {
  const mod = await getModule();
  return mod.ccall('wasm_has_watch_later', 'boolean', ['string'], [title]) as boolean;
}

export async function toggleLiked(movie: Movie): Promise<boolean> {
  const mod = await getModule();
  return mod.ccall('wasm_toggle_liked', 'boolean', [
    'string', 'number', 'string', 'string', 'string', 'string',
  ], [
    movie.title, movie.year, movie.genre, movie.director, movie.plot, movie.wiki_url,
  ]) as boolean;
}

export async function hasLiked(title: string): Promise<boolean> {
  const mod = await getModule();
  return mod.ccall('wasm_has_liked', 'boolean', ['string'], [title]) as boolean;
}

export async function clearUserData(): Promise<void> {
  const mod = await getModule();
  mod.ccall('wasm_clear_user_data', null, [], []);
}

// ── Persistence helpers (localStorage bridge) ───────────────────────────────
const STORAGE_KEY = 'streamc_user_data';

export async function persistUserData(): Promise<void> {
  const mod = await getModule();
  const json = mod.ccall('wasm_get_user_data_json', 'string', [], []) as string;
  localStorage.setItem(STORAGE_KEY, json);
}

export async function restoreUserData(): Promise<boolean> {
  const saved = localStorage.getItem(STORAGE_KEY);
  if (!saved) return false;
  return loadUserData(saved);
}
