(defvar current-id "") ;; item unique ID (you may wish to use it to tracks unique ID key)
(defvar current-index 0)
(defvar current-playlist nil)

(defun playlist-column-count-group (playlist-name row width height)
  (g-set-color |0.0 0.9 0.1 1.0|)
  (g-draw-text "test" 0 0 width height justification-left))

(defun playlist-column-count (playlist-name row width height)
  (defvar id (playlist-get playlist-name row 'id))
  (defvar play-count (ctags-get id 0))
  (defvar rank (ctags-get id 1))
  (defvar date (ctags-get id 2))
  (g-set-color |1.0 0.4 0.0 1.0|)
  (if (or play-count rank)
      (g-draw-text (strs "count: " (if play-count play-count "?")
                         " rank: " (if rank rank "?")
                         " " (if date (time-format date "%H:%M:%S %e-%m-%Y") ""))
                   0 0 width height justification-right)))

;; for given id returns new playlist
(defun init-playlist (id)
  ;;(message-box (strs "creating playlist: " id) "asdf")
  (create-playlist id)
  (playlist-add-column id "track" 'track nil 50 20 1000)
  (playlist-add-column id "album" 'album 'album 200 150 1000)
  (playlist-add-column id "artist" 'artist nil 200 150 1000)
  (playlist-add-column id "title" 'title nil 200 150 1000)
  (playlist-add-column id "year" 'year nil 70 50 1000)
  (playlist-add-column id "length" 'length nil 70 50 1000)
  (playlist-add-column id "count" 'playlist-column-count 'playlist-column-count-group 50 20 1000)
  (bind-mouse-double-click id 'on-playlist-click '(selected-row
                                                   selected-row-id
                                                   selected-row-index
                                                   component-name))
  id)

(defun create-playlist-tabs ()
  (create-tabs 'playlist-tabs 'top)
  ;;(tabs-add-component 'playlist-tabs (init-playlist 'playlist)
  ;;"all" |0.5 0.5 0.5 0.9|)
  'playlist-tabs)

(defun none ()
  (g-fill-all |0.2 0.2 0.2 1.0|))

(defun create-buttons ()
  (create-layout 'l-btns t)
  (create-text-button 'b-play ">" "Play")
  (create-text-button 'b-pause "||" "Pause")
  (create-text-button 'b-stop "[ ]" "Stop playback")
  (create-text-button 'b-prev "<<" "Previous")
  (create-text-button 'b-next ">>" "Next")
  (create-text-button 'b-rand "%" "Random")
  (create-text-button 'b-options "Options" "Audio options")
  (create-text-button 'b-interpreter "Interpreter" "GLISP Interpreter")
  (create-text-button 'b-new-playlist "New Playlist" "Create new playlist")
  (layout-add-component 'l-btns 'b-play 30.0 30.0 30.0)
  (layout-add-component 'l-btns 'b-pause 30.0 30.0 30.0)
  (layout-add-component 'l-btns 'b-stop 30.0 30.0 30.0)
  (layout-add-component 'l-btns 'b-prev 30.0 30.0 30.0)
  (layout-add-component 'l-btns 'b-next 30.0 30.0 30.0)
  (layout-add-component 'l-btns 'b-rand 30.0 30.0 30.0)
  (layout-add-component 'l-btns 'b-options 80.0 80.0 80.0)
  (layout-add-component 'l-btns 'b-interpreter 80.0 80.0 80.0)
  (layout-add-component 'l-btns 'b-new-playlist 80.0 80.0 80.0)
  (layout-add-component 'l-btns (create-panel 'nope 'none) 0.0 -1.0 -1.0)
  'l-btns)

(defun create-sliders ()
  (create-layout 'l-sliders t)
  (create-slider 'sl-seek)
  (component-enabled 'sl-seek nil)
  (create-slider 'sl-gain)
  (slider-range 'sl-gain 0.0 1.0)
  (slider-value 'sl-gain 1.0)
  (layout-add-component 'l-sliders 'sl-seek -0.1 -1.0 -1.0)
  (layout-add-component 'l-sliders 'sl-gain 100.0 100.0 100.0)
  'l-sliders)

(defun create-desc-info ()
  (create-layout 'l-desc-info nil)
  (create-panel 'pan 'none)
  (create-text-button 'bb ">" "Test")
  (layout-add-component 'l-desc-info 'pan -0.1 -1.0 -1.0)
  (layout-add-component 'l-desc-info (create-buttons) 20.0 20.0 20.0)
  (layout-add-component 'l-desc-info (create-sliders) 20.0 20.0 20.0)
  'l-desc-info)

(defun create-top-panel ()
  (create-layout 'l-top t)
  (create-image 'imm)
  (image-set-file 'imm "/home/calx/Downloads/front.jpg")
  (image-set-placement 'imm rect-placement-stretch)
  (layout-add-component 'l-top 'imm 150.0 150.0 15.0)
  (layout-add-component 'l-top (create-desc-info) -0.1 -0.5 -0.5)
  'l-top)

(create-layout 'l-main nil)
(layout-add-component 'l-main (create-top-panel) 150.0 150.0 150.0)
(layout-add-component 'l-main (create-playlist-tabs) 200.0 -1.0 -1.0)

;; callbacks
(defun spawn-audio-options ()
  (if (not (tabs-index 'playlist-tabs "Audio Options"))
      (progn
        (tabs-add-component 'playlist-tabs (audio-settings) "Audio Options" |0.9 0.5 0.5 0.9|)
        (tabs-index 'playlist-tabs "Audio Options"))))

(defun get-interpreter ()
  (if (has-component 'interpreter)
      'interpreter
    (create-interpreter 'interpreter)))

(defun spawn-interpreter ()
  (if (not (tabs-index 'playlist-tabs "Interpreter"))
      (progn
        (tabs-add-component 'playlist-tabs (get-interpreter) "Interpreter" |0.5 0.9 0.5 0.9|)
        (tabs-index 'playlist-tabs "Interpreter"))))

(defun create-new-playlist (name)
  (tabs-add-component 'playlist-tabs
                      (init-playlist (unique-id "playlist"))
                      name
                      |0.5 0.5 0.5 0.9|)
  (tabs-index 'playlist-tabs (- (tabs-count 'playlist-tabs) 1)))

(defun on-new-playlist ()
  (input-box "Playlist Name" "Enter new playlist name: " "New Playlist" 'create-new-playlist))

(defun on-stop ()
  (playback-stop)
  (playback-seek 0.0)
  (slider-value 'sl-seek 0.0)
  (component-enabled 'sl-seek nil))

(defun on-pause ()
  (playback-stop))

(defun on-play ()
  ;; TODO: fetch from playlist
  (component-enabled 'sl-seek t)
  (playback-start))

(defun toggle-playback ()
  (if (playback-is-playing)
      (on-pause)
    (on-play)))

(defun play-selected ()
  (setq current-id (playlist-get current-playlist current-index 'id))
  (playback-set-file (playlist-get current-playlist current-index 'path))
  (slider-range 'sl-seek 0.0 (playback-length)) ;; TODO: DRY
  (component-enabled 'sl-seek t) ;; TODO: playlist-select
  (playlist-select current-playlist current-index)
  (playback-start))

(defun on-next ()
  (if (< (+ 1 current-index) (playlist-items-count current-playlist))
      (progn (setq current-index (+ 1 current-index))
             (play-selected))))

(defun on-prev ()
  (if (>= (- current-index 1) 0)
      (progn (setq current-index (- current-index 1))
             (play-selected))))

(defun on-rand ()
  (progn (setq current-index (rand (playlist-items-count current-playlist)))
         (play-selected)))

(defun test-is-track ()
  (playlist-get current-playlist current-index 'is-track))

(defun on-playlist-click (item-str item-id item-index playlist-name)
  (playback-set-file item-str)
  (setq current-id item-id)
  (setq current-index item-index)
  (setq current-playlist playlist-name)
  (slider-range 'sl-seek 0.0 (playback-length))
  (component-enabled 'sl-seek t)
  (playback-start))

;; show explorer test (show-directory-in-explorer (extract-file-path "/home/calx/Downloads/somefile.png"))

(defun playback-changed ()
  (if (playback-is-playing)
      (start-timer 'update-slider 100)
    (stop-timer 'update-slider)
    (if (playback-finished)
        (progn
          (defvar current-value (ctags-get current-id 0))
          (defvar first-play (ctags-get current-id 2))
          (if current-value
                (ctags-set current-id 0 (+ 1 (ctags-get current-id 0))) ;; increase value
            (ctags-set current-id 0 1)) ;; init value
          (ctags-set current-id 3 (current-time)) ;; set timestamp
          (if (not first-play)
              (ctags-set current-id 2 (current-time))) ;; set first play date
          ;;(message-box "playback-changed callback" "playback finished")
          ;;(message-box "Current info" (strs current-playlist " : " current-index))
          (on-next)
          (repaint-component current-playlist)))))

(defun on-slider-up (time)
  (playback-seek time))

(defun on-update-slider ()
  (slider-value 'sl-seek (playback-get-pos)))

(defun on-slider-gain (new-gain)
  (playback-gain new-gain))

;; bindings
(bind-mouse-click 'b-options 'spawn-audio-options)
(bind-mouse-click 'b-interpreter 'spawn-interpreter)
(bind-mouse-click 'b-play 'on-play)
(bind-mouse-click 'b-pause 'on-pause)
(bind-mouse-click 'b-stop 'on-stop)
(bind-mouse-click 'b-prev 'on-prev)
(bind-mouse-click 'b-next 'on-next)
(bind-mouse-click 'b-rand 'on-rand)
(bind-mouse-click 'b-new-playlist 'on-new-playlist)
(bind-slider-drag-end 'sl-seek 'on-slider-up '(slider-value))
(bind-slider-changed 'sl-gain 'on-slider-gain '(slider-value))
(create-timer 'update-slider 'on-update-slider)
(bind-playback 'playback-changed)

;; initialize ctags
(ctags-load "playback-stats")
(settings-load "settings")

;; load playlist from settings
(setq current-index (or (settings-get "current-index") 0))
(setq current-playlist (or (settings-get "current-playlist") nil))
(dolist e (or (settings-get "playlist-tabs") '())
        ;;(message-box "playlist element: " (strs (nth 0 e) " / " (nth 1 e)))
        (tabs-add-component 'playlist-tabs
                            (init-playlist (nth 0 e))
                            (nth 1 e) |0.5 0.5 0.5 0.9|)
        (playlist-load (nth 0 e) (strs "playlists/" (nth 0 e))))

;; load volume
(if (settings-get "volume")
    (slider-value 'sl-gain (settings-get "volume")))

;; init audio settings
(audio-settings)

;; setup exit callback
(defun on-exit ()
  (defvar tabs (tabs-get-components 'playlist-tabs 'playlist))
  (dolist e tabs
          (playlist-save (nth 0 e) (strs "playlists/" (nth 0 e))))
  (settings-set "playlist-tabs" tabs)
  (settings-set "current-index" current-index)
  (settings-set "current-playlist" current-playlist)
  (settings-set "main-wnd-state" (main-window-state))
  (settings-set "volume" (slider-value 'sl-gain))
  (ctags-save "playback-stats")
  (settings-save "settings")
  ;;(message-box "Exiting" "just exiting")
  )
(bind-exit 'on-exit)

;; bind hotkeys
(bind-hotkey "Pause" "" 'toggle-playback)

;; filtered playlist test
(defun search (query)
  (create-filtered-playlist current-playlist 'ppp query)
  (playlist-add-column 'ppp "track" 'track nil 50 20 1000)
  (playlist-add-column 'ppp "album" 'album nil 200 150 1000)
  (playlist-add-column 'ppp "artist" 'artist nil 200 150 1000)
  (playlist-add-column 'ppp "title" 'title nil 200 150 1000)
  (playlist-add-column 'ppp "year" 'year nil 70 50 1000)
  ;;(playlist-add-column 'ppp "count" "0" 50 20 1000)
  ;;(playlist-add-column 'ppp "count" 'aaaaaaaaaaaaa 50 20 1000)
  ;;(create-window 'search-window "Search Results" |100.0 100.0 400.0 400.0| |1.0 1.0 1.0 1.0|)
  ;;(window-set-main-component 'search-window 'ppp)
  (tabs-set-component 'playlist-tabs 'ppp 0)
  )

(defun qqq ()
  (input-box "Search for:" "Enter query: " "" 'search))

(bind-key "F4" 'qqq)

;; TODO: replace current-playlist
(defun enable-filter (query)
  (playlist-enable-filter current-playlist query))

(defun on-f3 ()
  (if (playlist-filter-enabled current-playlist)
      (playlist-disable-filter current-playlist)
    (input-box "Quick search" "Query: " "" 'enable-filter)))

(defun on-n ()
  (if (playlist-filter-enabled current-playlist)
      (playlist-filter-next current-playlist)))

;; (bind-key "F3" 'on-f3)
;; (bind-key "F4" 'on-n)

;; make things visible
(set-main-component 'l-main)
(refresh-interface)

;; restore playlist tab and index
(if current-playlist
    (progn
      (tabs-index-component 'playlist-tabs current-playlist)
      (playlist-select current-playlist current-index)))

;; restore window layout and position
(if (settings-get "main-wnd-state")
    (main-window-state (settings-get "main-wnd-state")))
