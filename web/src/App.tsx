/**
 * App.tsx — Stream-C: Streaming platform powered by C++ WebAssembly
 *
 * Flow:
 *  1. WASM module loads → fetches CSV → C++ builds indexes (hash, suffix, tree)
 *  2. User searches / browses → JS calls wasm_search / wasm_browse_genre
 *  3. Likes & Watch Later → wasm_toggle_liked / wasm_toggle_watch_later
 *  4. Persistence via localStorage ↔ UserDataManager (C++)
 */

import { useCallback, useEffect, useRef, useState } from 'react'
import './App.css'
import type { Movie } from './wasm'
import {
  browseGenre,
  clearUserData,
  getFeatured,
  getGenres,
  getUserData,
  hasLiked,
  hasWatchLater,
  initEngine,
  persistUserData,
  restoreUserData,
  search,
  toggleLiked,
  toggleWatchLater,
} from './wasm'

// ── Types ────────────────────────────────────────────────────────────────────
type Page = 'home' | 'search' | 'liked' | 'watchlater'

// ── Helper: genre → emoji mapping ────────────────────────────────────────────
const genreEmojis: Record<string, string> = {
  action: '💥', adventure: '🗺️', comedy: '😂', drama: '🎭',
  horror: '👻', romance: '💕', thriller: '🔪', 'sci-fi': '🚀',
  'science fiction': '🚀', fantasy: '🧙', animation: '🎨',
  documentary: '📹', musical: '🎵', western: '🤠', crime: '🔍',
  mystery: '🕵️', war: '⚔️', biography: '📖', family: '👨‍👩‍👧‍👦',
  sport: '⚽', history: '📜', noir: '🌑', unknown: '🎬',
}

function getGenreEmoji(genre: string): string {
  const key = genre.toLowerCase().trim()
  for (const [k, v] of Object.entries(genreEmojis)) {
    if (key.includes(k)) return v
  }
  return '🎬'
}

// ── App ──────────────────────────────────────────────────────────────────────
function App() {
  // Engine state
  const [loading, setLoading] = useState(true)
  const [error, setError] = useState<string | null>(null)
  const [totalMovies, setTotalMovies] = useState(0)

  // Navigation
  const [page, setPage] = useState<Page>('home')
  const [scrolled, setScrolled] = useState(false)

  // Home page data
  const [featured, setFeatured] = useState<Movie[]>([])
  const [genres, setGenres] = useState<string[]>([])
  const [genreMovies, setGenreMovies] = useState<Record<string, Movie[]>>({})

  // Search
  const [searchQuery, setSearchQuery] = useState('')
  const [searchGenre, setSearchGenre] = useState('')
  const [searchYearFrom, setSearchYearFrom] = useState('')
  const [searchYearTo, setSearchYearTo] = useState('')
  const [searchResults, setSearchResults] = useState<Movie[]>([])
  const [searchOffset, setSearchOffset] = useState(0)
  const searchLimit = 20
  const searchInputRef = useRef<HTMLInputElement>(null)

  // User data (likes & watch later)
  const [likedSet, setLikedSet] = useState<Set<string>>(new Set())
  const [watchLaterSet, setWatchLaterSet] = useState<Set<string>>(new Set())
  const [likedMovies, setLikedMovies] = useState<Movie[]>([])
  const [watchLaterMovies, setWatchLaterMovies] = useState<Movie[]>([])

  // Modal
  const [selectedMovie, setSelectedMovie] = useState<Movie | null>(null)

  // ── Initialise engine ────────────────────────────────────────────────────
  useEffect(() => {
    let cancelled = false;

    (async () => {
      try {
        const count = await initEngine()
        if (cancelled) return
        setTotalMovies(count)

        // Restore user data from localStorage
        await restoreUserData()

        // Load home page data
        const [feat, g] = await Promise.all([getFeatured(12), getGenres()])
        if (cancelled) return
        setFeatured(feat)
        setGenres(g)

        // Load a few genre rows
        const topGenres = g.slice(0, 5)
        const genreData: Record<string, Movie[]> = {}
        await Promise.all(
          topGenres.map(async (genre) => {
            genreData[genre] = await browseGenre(genre, 0, 8)
          }),
        )
        if (cancelled) return
        setGenreMovies(genreData)

        // Sync liked/watchlater status
        await syncUserStatus(feat)

        setLoading(false)
      } catch (e) {
        if (cancelled) return
        setError(e instanceof Error ? e.message : String(e))
        setLoading(false)
      }
    })()

    return () => { cancelled = true }
  }, [])

  // ── Scroll listener for header ───────────────────────────────────────────
  useEffect(() => {
    const handleScroll = () => setScrolled(window.scrollY > 20)
    window.addEventListener('scroll', handleScroll, { passive: true })
    return () => window.removeEventListener('scroll', handleScroll)
  }, [])

  // ── Sync user status helper ──────────────────────────────────────────────
  const syncUserStatus = useCallback(async (movies: Movie[]) => {
    const liked = new Set<string>()
    const watchLater = new Set<string>()
    for (const m of movies) {
      if (await hasLiked(m.title)) liked.add(m.title)
      if (await hasWatchLater(m.title)) watchLater.add(m.title)
    }
    setLikedSet(liked)
    setWatchLaterSet(watchLater)
  }, [])

  // ── Search handler ───────────────────────────────────────────────────────
  const doSearch = useCallback(async (offset = 0) => {
    const results = await search({
      text: searchQuery || undefined,
      genre: searchGenre || undefined,
      yearFrom: searchYearFrom ? parseInt(searchYearFrom) : undefined,
      yearTo: searchYearTo ? parseInt(searchYearTo) : undefined,
      offset,
      limit: searchLimit,
    })
    setSearchResults(results)
    setSearchOffset(offset)
    setPage('search')
    await syncUserStatus(results)
  }, [searchQuery, searchGenre, searchYearFrom, searchYearTo, syncUserStatus])

  const handleSearchSubmit = useCallback((e: React.FormEvent | React.KeyboardEvent) => {
    e.preventDefault()
    doSearch(0)
  }, [doSearch])

  // ── Like / Watch Later handlers ──────────────────────────────────────────
  const handleToggleLike = useCallback(async (movie: Movie) => {
    const isNowLiked = await toggleLiked(movie)
    await persistUserData()
    setLikedSet(prev => {
      const next = new Set(prev)
      if (isNowLiked) next.add(movie.title)
      else next.delete(movie.title)
      return next
    })
  }, [])

  const handleToggleWatchLater = useCallback(async (movie: Movie) => {
    const isNowSaved = await toggleWatchLater(movie)
    await persistUserData()
    setWatchLaterSet(prev => {
      const next = new Set(prev)
      if (isNowSaved) next.add(movie.title)
      else next.delete(movie.title)
      return next
    })
  }, [])

  // ── Load liked / watch later pages ───────────────────────────────────────
  const loadUserPage = useCallback(async (which: 'liked' | 'watchlater') => {
    const data = await getUserData()
    if (which === 'liked') {
      const movies: Movie[] = data.liked.map((m, i) => ({
        id: i + 1,
        year: m.releaseYear,
        title: m.title,
        origin: m.origin,
        director: m.director,
        cast: m.cast,
        genre: m.genre,
        wiki_url: m.wikiPage,
        plot: m.plot,
        tags: [],
      }))
      setLikedMovies(movies)
    } else {
      const movies: Movie[] = data.watchLater.map((m, i) => ({
        id: i + 1,
        year: m.releaseYear,
        title: m.title,
        origin: m.origin,
        director: m.director,
        cast: m.cast,
        genre: m.genre,
        wiki_url: m.wikiPage,
        plot: m.plot,
        tags: [],
      }))
      setWatchLaterMovies(movies)
    }
    setPage(which)
  }, [])

  // ── Navigate ─────────────────────────────────────────────────────────────
  const goHome = useCallback(() => {
    setPage('home')
    window.scrollTo({ top: 0, behavior: 'smooth' })
  }, [])

  const handleClearData = useCallback(async () => {
    await clearUserData()
    await persistUserData()
    setLikedSet(new Set())
    setWatchLaterSet(new Set())
    setLikedMovies([])
    setWatchLaterMovies([])
  }, [])

  // ── Loading screen ───────────────────────────────────────────────────────
  if (loading) {
    return (
      <div className="loading-screen" id="loading-screen">
        <div className="loading-logo">Stream-C</div>
        <div className="loading-spinner" />
        <div className="loading-text">Cargando motor de búsqueda WebAssembly…</div>
        <div className="loading-progress">
          <div className="loading-progress-bar" />
        </div>
      </div>
    )
  }

  // ── Error screen ─────────────────────────────────────────────────────────
  if (error) {
    return (
      <div className="error-screen" id="error-screen">
        <div className="emoji">⚠️</div>
        <h2>Error al inicializar</h2>
        <p>
          No se pudo cargar el motor WebAssembly. Asegúrate de que los archivos
          <code>streaming.js</code> y <code>streaming.wasm</code> estén en
          <code>/public</code>, junto con <code>data_clean.csv</code>.
        </p>
        <p style={{ fontSize: 13, color: 'var(--text-dim)' }}>{error}</p>
        <button className="btn btn-primary" onClick={() => window.location.reload()}>
          🔄 Reintentar
        </button>
      </div>
    )
  }

  // ── Render ────────────────────────────────────────────────────────────────
  return (
    <div className="app">
      {/* ── Header ────────────────────────────────────────────────────────── */}
      <header className={`header ${scrolled ? 'header-scrolled' : ''}`} id="header">
        <div className="logo" onClick={goHome} id="logo">
          <div className="logo-icon">S</div>
          <span>Stream-C</span>
        </div>

        <nav className="nav-links" id="nav">
          <button
            className={`nav-link ${page === 'home' ? 'active' : ''}`}
            onClick={goHome}
            id="nav-home"
          >
            🏠 Inicio
          </button>
          <button
            className={`nav-link ${page === 'search' ? 'active' : ''}`}
            onClick={() => { setPage('search'); setTimeout(() => searchInputRef.current?.focus(), 100) }}
            id="nav-search"
          >
            🔍 Buscar
          </button>
          <button
            className={`nav-link ${page === 'liked' ? 'active' : ''}`}
            onClick={() => loadUserPage('liked')}
            id="nav-liked"
          >
            ❤️ Likes
          </button>
          <button
            className={`nav-link ${page === 'watchlater' ? 'active' : ''}`}
            onClick={() => loadUserPage('watchlater')}
            id="nav-watchlater"
          >
            🕐 Ver después
          </button>
        </nav>

        <div className="search-bar" id="search-bar">
          <span className="search-icon">🔍</span>
          <input
            ref={searchInputRef}
            type="text"
            placeholder="Buscar películas por título, trama…"
            value={searchQuery}
            onChange={(e) => setSearchQuery(e.target.value)}
            onKeyDown={(e) => { if (e.key === 'Enter') handleSearchSubmit(e) }}
            id="search-input"
          />
        </div>

        <div className="header-actions">
          <button
            className="header-btn"
            onClick={() => loadUserPage('liked')}
            title="Mis Likes"
            id="btn-likes"
          >
            ❤️
            {likedSet.size > 0 && <span className="badge">{likedSet.size}</span>}
          </button>
          <button
            className="header-btn"
            onClick={() => loadUserPage('watchlater')}
            title="Ver después"
            id="btn-watchlater"
          >
            🕐
            {watchLaterSet.size > 0 && <span className="badge">{watchLaterSet.size}</span>}
          </button>
        </div>
      </header>

      {/* ── Main content ──────────────────────────────────────────────────── */}
      <main className="main-content">
        {page === 'home' && (
          <HomePage
            totalMovies={totalMovies}
            featured={featured}
            genres={genres}
            genreMovies={genreMovies}
            likedSet={likedSet}
            watchLaterSet={watchLaterSet}
            onSelectMovie={setSelectedMovie}
            onToggleLike={handleToggleLike}
            onToggleWatchLater={handleToggleWatchLater}
            onBrowseGenre={(g) => { setSearchGenre(g); setSearchQuery(''); doSearch(0) }}
          />
        )}

        {page === 'search' && (
          <SearchPage
            query={searchQuery}
            genre={searchGenre}
            yearFrom={searchYearFrom}
            yearTo={searchYearTo}
            results={searchResults}
            offset={searchOffset}
            limit={searchLimit}
            genres={genres}
            likedSet={likedSet}
            watchLaterSet={watchLaterSet}
            onQueryChange={setSearchQuery}
            onGenreChange={setSearchGenre}
            onYearFromChange={setSearchYearFrom}
            onYearToChange={setSearchYearTo}
            onSearch={doSearch}
            onSelectMovie={setSelectedMovie}
            onToggleLike={handleToggleLike}
            onToggleWatchLater={handleToggleWatchLater}
          />
        )}

        {page === 'liked' && (
          <UserPage
            title="❤️ Películas con Like"
            description="Las películas que te han gustado"
            movies={likedMovies}
            emptyEmoji="❤️"
            emptyText="Aún no has dado like a ninguna película"
            likedSet={likedSet}
            watchLaterSet={watchLaterSet}
            onSelectMovie={setSelectedMovie}
            onToggleLike={handleToggleLike}
            onToggleWatchLater={handleToggleWatchLater}
            onClear={handleClearData}
          />
        )}

        {page === 'watchlater' && (
          <UserPage
            title="🕐 Ver después"
            description="Películas guardadas para ver más tarde"
            movies={watchLaterMovies}
            emptyEmoji="🕐"
            emptyText="No has guardado películas para ver después"
            likedSet={likedSet}
            watchLaterSet={watchLaterSet}
            onSelectMovie={setSelectedMovie}
            onToggleLike={handleToggleLike}
            onToggleWatchLater={handleToggleWatchLater}
            onClear={handleClearData}
          />
        )}
      </main>

      {/* ── Footer ────────────────────────────────────────────────────────── */}
      <footer className="footer" id="footer">
        <div className="footer-content">
          <span>© 2024 Stream-C — Potenciado por C++ y WebAssembly</span>
          <div className="footer-tech">
            <span className="footer-tech-badge">C++17</span>
            <span className="footer-tech-badge">WebAssembly</span>
            <span className="footer-tech-badge">React + Vite</span>
            <span className="footer-tech-badge">{totalMovies.toLocaleString()} películas</span>
          </div>
        </div>
      </footer>

      {/* ── Movie detail modal ────────────────────────────────────────────── */}
      {selectedMovie && (
        <MovieModal
          movie={selectedMovie}
          isLiked={likedSet.has(selectedMovie.title)}
          isWatchLater={watchLaterSet.has(selectedMovie.title)}
          onClose={() => setSelectedMovie(null)}
          onToggleLike={handleToggleLike}
          onToggleWatchLater={handleToggleWatchLater}
        />
      )}
    </div>
  )
}

// ═══════════════════════════════════════════════════════════════════════════════
// Sub-components
// ═══════════════════════════════════════════════════════════════════════════════

// ── Home Page ────────────────────────────────────────────────────────────────
interface HomePageProps {
  totalMovies: number
  featured: Movie[]
  genres: string[]
  genreMovies: Record<string, Movie[]>
  likedSet: Set<string>
  watchLaterSet: Set<string>
  onSelectMovie: (m: Movie) => void
  onToggleLike: (m: Movie) => void
  onToggleWatchLater: (m: Movie) => void
  onBrowseGenre: (g: string) => void
}

function HomePage({
  totalMovies, featured, genres, genreMovies, likedSet, watchLaterSet,
  onSelectMovie, onToggleLike, onToggleWatchLater, onBrowseGenre,
}: HomePageProps) {
  return (
    <>
      {/* Hero */}
      <section className="hero" id="hero">
        <div className="hero-content">
          <div className="hero-badge">
            <span className="dot" />
            Motor de búsqueda C++ con WebAssembly
          </div>
          <h1>
            Descubre miles de <br />
            <span className="gradient-text">películas al instante</span>
          </h1>
          <p>
            Búsqueda ultra-rápida potenciada por hashing O(1), suffix arrays,
            y árboles BST — todo corriendo directamente en tu navegador.
          </p>
          <div className="hero-stats">
            <div className="hero-stat">
              <div className="value">{totalMovies.toLocaleString()}</div>
              <div className="label">Películas</div>
            </div>
            <div className="hero-stat">
              <div className="value">O(1)</div>
              <div className="label">Hash Lookup</div>
            </div>
            <div className="hero-stat">
              <div className="value">O(n·log·n)</div>
              <div className="label">Suffix Search</div>
            </div>
          </div>
        </div>
      </section>

      {/* Featured */}
      <section className="section" id="featured-section">
        <div className="section-header">
          <h2 className="section-title">
            <span className="icon">⭐</span> Destacadas
          </h2>
        </div>
        <div className="movie-grid stagger">
          {featured.map((m) => (
            <MovieCard
              key={m.id}
              movie={m}
              isLiked={likedSet.has(m.title)}
              isWatchLater={watchLaterSet.has(m.title)}
              onClick={() => onSelectMovie(m)}
              onToggleLike={() => onToggleLike(m)}
              onToggleWatchLater={() => onToggleWatchLater(m)}
            />
          ))}
        </div>
      </section>

      {/* Genres */}
      <section className="section" id="genres-section">
        <div className="section-header">
          <h2 className="section-title">
            <span className="icon">🎭</span> Explorar por género
          </h2>
        </div>
        <div className="genre-chips">
          {genres.slice(0, 20).map((g) => (
            <button
              key={g}
              className="genre-chip"
              onClick={() => onBrowseGenre(g)}
            >
              {getGenreEmoji(g)} {g}
            </button>
          ))}
        </div>
      </section>

      {/* Genre rows */}
      {Object.entries(genreMovies).map(([genre, movies]) => (
        <section className="section" key={genre}>
          <div className="section-header">
            <h2 className="section-title">
              <span className="icon">{getGenreEmoji(genre)}</span> {genre}
            </h2>
            <button className="section-action" onClick={() => onBrowseGenre(genre)}>
              Ver todo →
            </button>
          </div>
          <div className="movie-grid stagger">
            {movies.map((m) => (
              <MovieCard
                key={m.id}
                movie={m}
                isLiked={likedSet.has(m.title)}
                isWatchLater={watchLaterSet.has(m.title)}
                onClick={() => onSelectMovie(m)}
                onToggleLike={() => onToggleLike(m)}
                onToggleWatchLater={() => onToggleWatchLater(m)}
              />
            ))}
          </div>
        </section>
      ))}
    </>
  )
}

// ── Search Page ──────────────────────────────────────────────────────────────
interface SearchPageProps {
  query: string
  genre: string
  yearFrom: string
  yearTo: string
  results: Movie[]
  offset: number
  limit: number
  genres: string[]
  likedSet: Set<string>
  watchLaterSet: Set<string>
  onQueryChange: (v: string) => void
  onGenreChange: (v: string) => void
  onYearFromChange: (v: string) => void
  onYearToChange: (v: string) => void
  onSearch: (offset?: number) => void
  onSelectMovie: (m: Movie) => void
  onToggleLike: (m: Movie) => void
  onToggleWatchLater: (m: Movie) => void
}

function SearchPage({
  query, genre, yearFrom, yearTo, results, offset, limit, genres,
  likedSet, watchLaterSet,
  onQueryChange, onGenreChange, onYearFromChange, onYearToChange,
  onSearch, onSelectMovie, onToggleLike, onToggleWatchLater,
}: SearchPageProps) {
  return (
    <div className="search-page" id="search-page">
      <div className="user-page-header fade-in">
        <h1>🔍 Búsqueda avanzada</h1>
        <p>Busca por texto, género y rango de años — todo procesado en C++ WebAssembly</p>
      </div>

      <div className="search-filters fade-in">
        <input
          className="filter-input"
          style={{ flex: 1, minWidth: 200 }}
          type="text"
          placeholder="Título o trama…"
          value={query}
          onChange={(e) => onQueryChange(e.target.value)}
          onKeyDown={(e) => { if (e.key === 'Enter') onSearch(0) }}
          id="filter-text"
        />
        <select
          className="filter-select"
          value={genre}
          onChange={(e) => onGenreChange(e.target.value)}
          id="filter-genre"
        >
          <option value="">Todos los géneros</option>
          {genres.map((g) => (
            <option key={g} value={g}>{g}</option>
          ))}
        </select>
        <input
          className="filter-input"
          type="number"
          placeholder="Año desde"
          value={yearFrom}
          onChange={(e) => onYearFromChange(e.target.value)}
          id="filter-year-from"
        />
        <input
          className="filter-input"
          type="number"
          placeholder="Año hasta"
          value={yearTo}
          onChange={(e) => onYearToChange(e.target.value)}
          id="filter-year-to"
        />
        <button className="btn btn-primary" onClick={() => onSearch(0)} id="btn-search">
          🔍 Buscar
        </button>
      </div>

      <div className="search-results-info fade-in">
        <span>
          Mostrando <span className="search-results-count">{results.length}</span> resultados
          {offset > 0 && ` (página ${Math.floor(offset / limit) + 1})`}
        </span>
      </div>

      {results.length > 0 ? (
        <>
          <div className="movie-grid stagger">
            {results.map((m) => (
              <MovieCard
                key={m.id}
                movie={m}
                isLiked={likedSet.has(m.title)}
                isWatchLater={watchLaterSet.has(m.title)}
                onClick={() => onSelectMovie(m)}
                onToggleLike={() => onToggleLike(m)}
                onToggleWatchLater={() => onToggleWatchLater(m)}
              />
            ))}
          </div>
          <div className="pagination">
            <button
              className="pagination-btn"
              disabled={offset === 0}
              onClick={() => onSearch(Math.max(0, offset - limit))}
            >
              ← Anterior
            </button>
            <span className="pagination-info">
              Página {Math.floor(offset / limit) + 1}
            </span>
            <button
              className="pagination-btn"
              disabled={results.length < limit}
              onClick={() => onSearch(offset + limit)}
            >
              Siguiente →
            </button>
          </div>
        </>
      ) : (
        <div className="empty-state fade-in">
          <div className="emoji">🔍</div>
          <h3>Sin resultados</h3>
          <p>Intenta con otros términos de búsqueda o ajusta los filtros</p>
        </div>
      )}
    </div>
  )
}

// ── User Page (Liked / Watch Later) ──────────────────────────────────────────
interface UserPageProps {
  title: string
  description: string
  movies: Movie[]
  emptyEmoji: string
  emptyText: string
  likedSet: Set<string>
  watchLaterSet: Set<string>
  onSelectMovie: (m: Movie) => void
  onToggleLike: (m: Movie) => void
  onToggleWatchLater: (m: Movie) => void
  onClear: () => void
}

function UserPage({
  title, description, movies, emptyEmoji, emptyText,
  likedSet, watchLaterSet,
  onSelectMovie, onToggleLike, onToggleWatchLater, onClear,
}: UserPageProps) {
  return (
    <div className="user-page" id="user-page">
      <div className="user-page-header fade-in">
        <h1>{title}</h1>
        <p>{description}</p>
        {movies.length > 0 && (
          <button
            className="btn btn-secondary"
            style={{ marginTop: 16 }}
            onClick={onClear}
          >
            🗑️ Limpiar todo
          </button>
        )}
      </div>

      {movies.length > 0 ? (
        <div className="movie-grid stagger">
          {movies.map((m, i) => (
            <MovieCard
              key={`${m.title}-${i}`}
              movie={m}
              isLiked={likedSet.has(m.title)}
              isWatchLater={watchLaterSet.has(m.title)}
              onClick={() => onSelectMovie(m)}
              onToggleLike={() => onToggleLike(m)}
              onToggleWatchLater={() => onToggleWatchLater(m)}
            />
          ))}
        </div>
      ) : (
        <div className="empty-state fade-in">
          <div className="emoji">{emptyEmoji}</div>
          <h3>{emptyText}</h3>
          <p>Explora el catálogo y comienza a guardar tus favoritas</p>
        </div>
      )}
    </div>
  )
}

// ── Movie Card ───────────────────────────────────────────────────────────────
interface MovieCardProps {
  movie: Movie
  isLiked: boolean
  isWatchLater: boolean
  onClick: () => void
  onToggleLike: () => void
  onToggleWatchLater: () => void
}

function MovieCard({
  movie, isLiked, isWatchLater,
  onClick, onToggleLike, onToggleWatchLater,
}: MovieCardProps) {
  return (
    <div className="movie-card" onClick={onClick}>
      <div className="movie-poster">
        <div className="movie-poster-placeholder">
          <span className="emoji">{getGenreEmoji(movie.genre)}</span>
          <span className="year">{movie.year || '—'}</span>
        </div>
        <div className="movie-card-overlay">
          <div className="movie-card-actions">
            <button
              className={`card-action-btn ${isLiked ? 'active' : ''}`}
              onClick={(e) => { e.stopPropagation(); onToggleLike() }}
              title={isLiked ? 'Quitar like' : 'Dar like'}
            >
              {isLiked ? '❤️' : '🤍'}
            </button>
            <button
              className={`card-action-btn ${isWatchLater ? 'active' : ''}`}
              onClick={(e) => { e.stopPropagation(); onToggleWatchLater() }}
              title={isWatchLater ? 'Quitar de ver después' : 'Ver después'}
            >
              {isWatchLater ? '✅' : '🕐'}
            </button>
          </div>
        </div>
      </div>
      <div className="movie-info">
        <div className="movie-title">{movie.title}</div>
        <div className="movie-meta">
          {movie.year > 0 && <span className="movie-year">{movie.year}</span>}
          {movie.genre && movie.genre !== 'unknown' && (
            <span className="movie-genre-tag">{movie.genre}</span>
          )}
        </div>
      </div>
    </div>
  )
}

// ── Movie Modal ──────────────────────────────────────────────────────────────
interface MovieModalProps {
  movie: Movie
  isLiked: boolean
  isWatchLater: boolean
  onClose: () => void
  onToggleLike: (m: Movie) => void
  onToggleWatchLater: (m: Movie) => void
}

function MovieModal({
  movie, isLiked, isWatchLater, onClose, onToggleLike, onToggleWatchLater,
}: MovieModalProps) {
  // Close on ESC
  useEffect(() => {
    const handler = (e: KeyboardEvent) => { if (e.key === 'Escape') onClose() }
    window.addEventListener('keydown', handler)
    return () => window.removeEventListener('keydown', handler)
  }, [onClose])

  return (
    <div className="modal-backdrop" onClick={onClose} id="movie-modal">
      <div className="modal-content" onClick={(e) => e.stopPropagation()}>
        <div className="modal-hero">
          <button className="modal-close" onClick={onClose}>✕</button>
          <h2 className="modal-title">{movie.title}</h2>
          <div className="modal-subtitle">
            {movie.year > 0 && <span>{movie.year}</span>}
            {movie.genre && movie.genre !== 'unknown' && (
              <>
                <span className="divider" />
                <span>{movie.genre}</span>
              </>
            )}
            {movie.director && movie.director !== 'unknown' && (
              <>
                <span className="divider" />
                <span>🎬 {movie.director}</span>
              </>
            )}
            {movie.origin && movie.origin !== 'unknown' && (
              <>
                <span className="divider" />
                <span>🌍 {movie.origin}</span>
              </>
            )}
          </div>
        </div>

        <div className="modal-body">
          {movie.plot && movie.plot !== 'unknown' && (
            <div className="modal-section">
              <div className="modal-section-title">Sinopsis</div>
              <p className="modal-plot">{movie.plot}</p>
            </div>
          )}

          {movie.cast && movie.cast !== 'unknown' && (
            <div className="modal-section">
              <div className="modal-section-title">Reparto</div>
              <p className="modal-plot">{movie.cast}</p>
            </div>
          )}

          {movie.tags && movie.tags.length > 0 && (
            <div className="modal-section">
              <div className="modal-section-title">Tags (generados por C++)</div>
              <div className="modal-tags">
                {movie.tags.map((t) => (
                  <span key={t} className="modal-tag">{t}</span>
                ))}
              </div>
            </div>
          )}

          {movie.wiki_url && movie.wiki_url !== 'unknown' && (
            <div className="modal-section">
              <div className="modal-section-title">Más información</div>
              <a href={movie.wiki_url} target="_blank" rel="noopener noreferrer">
                📖 Ver en Wikipedia →
              </a>
            </div>
          )}
        </div>

        <div className="modal-actions">
          <button
            className={`btn btn-primary ${isLiked ? 'active' : ''}`}
            onClick={() => onToggleLike(movie)}
          >
            {isLiked ? '❤️ Te gusta' : '🤍 Me gusta'}
          </button>
          <button
            className={`btn btn-secondary ${isWatchLater ? 'active' : ''}`}
            onClick={() => onToggleWatchLater(movie)}
          >
            {isWatchLater ? '✅ Guardada' : '🕐 Ver después'}
          </button>
        </div>
      </div>
    </div>
  )
}

export default App
